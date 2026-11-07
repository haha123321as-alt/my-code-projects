#include "ipc.h"
#include "mq_ipc.h"

/*
POSIX Message Queue (Hàng đợi thông điệp):
- Khác với Socket hay FIFO (Pipe) truyền dữ liệu dạng luồng byte (Byte Stream),
  Message Queue truyền nhận dữ liệu theo từng "thông điệp" (Message-oriented) độc lập.
- Mỗi thông điệp có ranh giới rõ ràng, không cần người dùng tự ghép/tách byte bằng ký tự '\n'.
- Mỗi thông điệp có thể gán mức độ ưu tiên (priority). Mức ưu tiên cao hơn sẽ được nhận trước.
- Hàng đợi tồn tại trong kernel của hệ điều hành cho đến khi gọi mq_unlink() hoặc khởi động lại máy.
- Tên của POSIX Message Queue phải bắt đầu bằng ký tự '/' (ví dụ: "/ipc_task1_mq").
*/

// == SENDER ==
int mq_sender(void)
{
    char buf[BUFFER_SIZE];
    printf("[SENDER] Waiting for MQ receiver...\n");

    /*
    mq_open():
    - Mở một POSIX Message Queue đã tồn tại để ghi (O_WRONLY).
    - Trả về mqd_t (Message Queue Descriptor) tương tự file descriptor.
    - Thất bại trả về (mqd_t)-1.
    */
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        if (ipc_running) {
            perror("mq_open");
            printf("[SENDER] Please ensure receiver is running first to create the queue.\n");
        } else {
            printf("[SENDER] Program stopped.\n");
        }
        return -1;
    }

    printf("[SENDER] MQ connected!\n");

    while (ipc_running) {
        printf("[SENDER] : ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            if (!ipc_running || feof(stdin)) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        // Xóa ký tự newline '\n' ở cuối chuỗi
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[--len] = '\0';
        }

        // Bỏ qua nếu nhập chuỗi rỗng
        if (len == 0) {
            continue;
        }

        /*
        mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio):
        - Gửi thông điệp vào queue.
        - msg_len: Độ dài dữ liệu gửi (ở đây gửi cả ký tự '\0' kết thúc chuỗi nên là len + 1).
        - msg_prio: Mức độ ưu tiên của tin nhắn (0 là mặc định, số càng lớn ưu tiên càng cao).
        - Trả về 0 nếu thành công, -1 nếu thất bại (ví dụ: EINTR khi nhận signal).
        */
        int sent = 0;
        while (ipc_running && !sent) {
            if (mq_send(mq, buf, len + 1, 0) == -1) {
                if (errno == EINTR) {
                    continue; // Bị ngắt bởi signal, thử lại nếu chương trình còn chạy
                }
                perror("mq_send");
                break;
            }
            sent = 1;
        }

        if (!sent && ipc_running) {
            break;
        }
    }

    // Đóng Message Queue descriptor
    mq_close(mq);
    printf("\n[SENDER] MQ disconnected.\n");
    return 0;
}

// == RECEIVER ==
int mq_receiver(void)
{
    // Xóa hàng đợi cũ nếu còn sót lại từ lần chạy trước
    mq_unlink(MQ_NAME);

    /*
    Cấu hình thuộc tính cho Message Queue:
    - mq_flags: 0 (hoặc O_NONBLOCK nếu không muốn chờ)
    - mq_maxmsg: Số lượng thông điệp tối đa được chứa trong hàng đợi
    - mq_msgsize: Kích thước tối đa của mỗi thông điệp (byte)
    - mq_curmsgs: Số lượng thông điệp hiện có (bị bỏ qua khi gọi mq_open)
    */
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFFER_SIZE;
    attr.mq_curmsgs = 0;

    /*
    mq_open() với O_CREAT:
    - Tạo mới message queue nếu chưa có.
    - 0666: Quyền truy cập (đọc/ghi cho user, group, other).
    - &attr: Cấu hình thuộc tính hàng đợi vừa tạo.
    */
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return -1;
    }

    printf("[RECEIVER] MQ created successfully.\n");
    printf("[RECEIVER] Waiting for messages...\n");
    fflush(stdout);

    char buffer[BUFFER_SIZE];
    unsigned int prio = 0;

    while (ipc_running) {
        /*
        mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio):
        - Nhận một thông điệp từ hàng đợi.
        - LƯU Ý QUAN TRỌNG: msg_len PHẢI >= attr.mq_msgsize, nếu không sẽ bị lỗi EMSGSIZE.
        - msg_prio: Lưu độ ưu tiên của tin nhắn vừa nhận (hoặc NULL nếu không quan tâm).
        - Trả về số byte đọc được, hoặc -1 nếu thất bại.
        */
        ssize_t bytes_read = mq_receive(mq, buffer, sizeof(buffer), &prio);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue; // Bị ngắt bởi SIGINT (Ctrl+C), vòng lặp sẽ kiểm tra ipc_running và thoát
            }
            if (ipc_running) {
                perror("mq_receive");
            }
            break;
        }

        // Đảm bảo chuỗi kết thúc bằng '\0' an toàn
        if ((size_t)bytes_read < sizeof(buffer)) {
            buffer[bytes_read] = '\0';
        } else {
            buffer[sizeof(buffer) - 1] = '\0';
        }

        printf("[RECEIVER] Received: %s\n", buffer);
        fflush(stdout);
    }

    // Đóng descriptor và xóa hàng đợi khỏi kernel
    mq_close(mq);
    mq_unlink(MQ_NAME);
    printf("\n[RECEIVER] MQ receiver stopped.\n");
    return 0;
}


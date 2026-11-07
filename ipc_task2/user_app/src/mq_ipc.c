/*
 * mq_ipc.c - Truyền thông điệp qua POSIX Message Queue
 *
 * Sender nhận character/word từ keyboard driver qua signal, sau đó đóng gói
 * thành thông điệp gửi vào POSIX Message Queue (/ipc_task2_mq).
 * Receiver lấy thông điệp từ hàng đợi và in ra màn hình.
 */

#include "ipc.h"
#include "mq_ipc.h"

/*
 * =========================================================================
 * POSIX MESSAGE QUEUE SENDER
 * =========================================================================
 */
int mq_sender(int dev_fd)
{
    char buf[BUFFER_SIZE];
    printf("[SENDER] Waiting for Message Queue (%s)...\n", MQ_NAME);

    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        if (ipc_running) {
            perror("[SENDER] mq_open");
            printf("[SENDER] Please ensure receiver is running first: ./build/receiver mq\n");
        }
        return -1;
    }

    printf("[SENDER] Message Queue connected!\n");
    printf("[SENDER] Type on your keyboard (or write to /dev/keyboard_dev) to send words!\n");

    while (ipc_running) {
        int len = kbd_get_word(dev_fd, buf, sizeof(buf));
        if (len < 0) {
            if (!ipc_running) break;
            if (errno == EINTR) continue;
            break;
        }

        if (len == 0) {
            continue;
        }

        /*
         * Gửi thông điệp vào queue.
         * Độ dài thông điệp: len + 1 (để truyền cả ký tự '\0' kết thúc chuỗi).
         */
        int sent = 0;
        while (ipc_running && !sent) {
            if (mq_send(mq, buf, (size_t)len + 1, 0) == -1) {
                if (errno == EINTR) {
                    continue; /* Bị ngắt bởi signal, tiếp tục gửi lại */
                }
                perror("[SENDER] mq_send");
                break;
            }
            sent = 1;
        }

        if (!sent && ipc_running) {
            break;
        }

        printf("[SENDER] -> Sent to receiver via Message Queue: \"%s\"\n", buf);
    }

    mq_close(mq);
    printf("\n[SENDER] Message Queue disconnected.\n");
    return 0;
}

/*
 * =========================================================================
 * POSIX MESSAGE QUEUE RECEIVER
 * =========================================================================
 */
int mq_receiver(void)
{
    /* Xóa hàng đợi cũ nếu còn sót lại trước khi tạo mới */
    mq_unlink(MQ_NAME);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFFER_SIZE;
    attr.mq_curmsgs = 0;

    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("[RECEIVER] mq_open");
        return -1;
    }

    printf("[RECEIVER] Message Queue created successfully at %s\n", MQ_NAME);
    printf("[RECEIVER] Waiting for messages from Sender...\n");
    fflush(stdout);

    char buffer[BUFFER_SIZE];
    unsigned int prio = 0;

    while (ipc_running) {
        /*
         * mq_receive: Nhận một thông điệp từ hàng đợi.
         * Chú ý: buffer size bắt buộc phải >= attr.mq_msgsize.
         */
        ssize_t bytes_read = mq_receive(mq, buffer, sizeof(buffer), &prio);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue; /* Bị ngắt bởi SIGINT (Ctrl+C), kiểm tra ipc_running */
            }
            if (ipc_running) {
                perror("[RECEIVER] mq_receive");
            }
            break;
        }

        if ((size_t)bytes_read < sizeof(buffer)) {
            buffer[bytes_read] = '\0';
        } else {
            buffer[sizeof(buffer) - 1] = '\0';
        }

        printf("[RECEIVER] Received: %s\n", buffer);
        fflush(stdout);
    }

    mq_close(mq);
    mq_unlink(MQ_NAME);
    printf("\n[RECEIVER] Message Queue receiver stopped.\n");
    return 0;
}


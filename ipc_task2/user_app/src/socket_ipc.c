/*
 * socket_ipc.c - Truyền thông điệp qua Unix Domain Socket (UDS)
 *
 * Tương tự Task 1, Sender gửi dữ liệu (character/word) nhận từ keyboard driver
 * tới Receiver qua Unix Domain Socket dạng Stream (SOCK_STREAM).
 */

#include "ipc.h"
#include "socket_ipc.h"

/*
 * Ghi toàn bộ dữ liệu vào file descriptor, xử lý lỗi EINTR nếu bị gián đoạn bởi signal
 */
static int write_all(int fd, const char *data, size_t len)
{
    size_t total = 0;
    while (total < len && ipc_running) {
        ssize_t n = write(fd, data + total, len - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue; /* Bị ngắt bởi signal, tiếp tục ghi phần còn lại */
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        total += n;
    }
    return ipc_running ? 0 : -1;
}

/*
 * =========================================================================
 * SOCKET SENDER
 * =========================================================================
 * 1. Tạo socket AF_UNIX kiểu SOCK_STREAM.
 * 2. Kết nối tới Receiver tại đường dẫn SOCKET_PATH (/tmp/ipc_task2_socket).
 * 3. Vòng lặp: Gọi kbd_get_word() nhận từ bàn phím từ driver qua signal,
 *    sau đó gửi sang Receiver qua socket.
 */
int socket_sender(int dev_fd)
{
    int s;
    struct sockaddr_un addr;
    char buf[BUFFER_SIZE];

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        perror("[SENDER] socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    printf("[SENDER] Connecting to socket receiver at %s...\n", SOCKET_PATH);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[SENDER] connect");
        printf("[SENDER] Make sure receiver is running first: ./build/receiver socket\n");
        close(s);
        return -1;
    }

    printf("[SENDER] Socket connected successfully!\n");
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

        /* Gửi chuỗi ký tự nhận được qua socket */
        if (write_all(s, buf, (size_t)len) < 0) {
            if (ipc_running) perror("[SENDER] socket write");
            break;
        }

        /* Gửi kèm ký tự xuống dòng '\n' để báo hiệu kết thúc một thông điệp */
        if (write_all(s, "\n", 1) < 0) {
            if (ipc_running) perror("[SENDER] socket write newline");
            break;
        }

        printf("[SENDER] -> Sent to receiver via Socket: \"%s\"\n", buf);
    }

    close(s);
    printf("\n[SENDER] Socket disconnected.\n");
    return 0;
}

/*
 * =========================================================================
 * SOCKET RECEIVER
 * =========================================================================
 * 1. Tạo socket AF_UNIX.
 * 2. Bind vào SOCKET_PATH và lắng nghe kết nối (listen).
 * 3. Chấp nhận kết nối (accept) từ Sender.
 * 4. Đọc dữ liệu dạng byte stream, tách thông điệp bằng ký tự '\n' và in ra màn hình.
 */
int socket_receiver(void)
{
    int s;
    struct sockaddr_un addr;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        perror("[RECEIVER] socket");
        return -1;
    }

    /* Xóa file socket cũ nếu còn sót lại trước khi bind */
    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[RECEIVER] bind");
        close(s);
        return -1;
    }

    if (listen(s, 5) < 0) {
        perror("[RECEIVER] listen");
        close(s);
        unlink(SOCKET_PATH);
        return -1;
    }

    printf("[RECEIVER] Socket server started, listening on %s...\n", SOCKET_PATH);

    while (ipc_running) {
        int client_fd = accept(s, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            if (ipc_running) perror("[RECEIVER] accept");
            break;
        }

        printf("[RECEIVER] Sender connected via Socket!\n");

        char buf[BUFFER_SIZE];
        size_t pos = 0;
        char ch;
        ssize_t n;

        while (ipc_running) {
            n = read(client_fd, &ch, 1);
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (n == 0) {
                break; /* Sender đã ngắt kết nối */
            }

            if (ch == '\n') {
                buf[pos] = '\0';
                printf("[RECEIVER] Received: %s\n", buf);
                fflush(stdout);
                pos = 0;
            } else if (pos < BUFFER_SIZE - 1) {
                buf[pos++] = ch;
            } else {
                printf("[RECEIVER] Message too long. Discarding.\n");
                pos = 0;
            }
        }

        if (pos > 0) {
            buf[pos] = '\0';
            printf("[RECEIVER] Received: %s\n", buf);
        }

        close(client_fd);
        printf("[RECEIVER] Sender disconnected.\n");
    }

    close(s);
    unlink(SOCKET_PATH);
    printf("[RECEIVER] Socket server stopped.\n");
    return 0;
}


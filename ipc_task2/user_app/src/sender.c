/*
 * sender.c - Sender Application (Task 2)
 */

#include "ipc.h"
#include "socket_ipc.h"
#include "fifo_ipc.h"
#include "mq_ipc.h"

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        printf("Usage: ./sender [socket|fifo|mq] [word|char]\n");
        printf("  - mode 'word' (default): Go chu, an SPACE hoac ENTER la gui luon ca tu!\n");
        printf("  - mode 'char':           Go bat ky phim nao se gui ngay tuc thi sang receiver!\n");
        return 1;
    }

    /* Kiểm tra chế độ gửi: word (mặc định) hay char */
    if (argc == 3 && strcmp(argv[2], "char") == 0) {
        kbd_send_mode = MODE_CHAR;
    } else {
        kbd_send_mode = MODE_WORD;
    }

    printf("========================================================\n");
    printf("   Task 2: Keyboard Driver Intercept & Sender IPC       \n");
    printf("========================================================\n");
    printf("[SENDER] Mode: %s\n", (kbd_send_mode == MODE_CHAR) 
           ? "CHARACTER (Moi phim bam gui ngay sang Receiver!)" 
           : "WORD (Go xong tu, an SPACE hoac ENTER de gui!)");

    /* 1. Khởi tạo bắt signal hệ thống (SIGINT, SIGTERM, SIGPIPE) */
    ipc_init_signals();

    /* 2. Đăng ký signal handler cho SIGUSR1 với SA_SIGINFO để nhận dữ liệu từ kernel */
    kbd_init_signal_handler();

    /* 3. Mở character device driver /dev/keyboard_dev */
    int dev_fd = kbd_open_driver();

    /* 
     * Tắt Terminal Echo để tránh lặp ký tự (double echo: ss, ll)
     * Đăng ký atexit để luôn khôi phục terminal bình thường khi thoát
     */
    atexit(terminal_restore_echo);
    if (dev_fd >= 0) {
        terminal_disable_echo();
    }

    printf("[SENDER] Starting IPC sender with channel: %s\n", argv[1]);

    /* 4. Khởi chạy sender theo phương thức IPC được chọn */
    if (strcmp(argv[1], "socket") == 0) {
        socket_sender(dev_fd);
    }
    else if (strcmp(argv[1], "fifo") == 0) {
        fifo_sender(dev_fd);
    }
    else if (strcmp(argv[1], "mq") == 0 || strcmp(argv[1], "queue") == 0) {
        mq_sender(dev_fd);
    }
    else {
        printf("Invalid communication mode! Use 'socket', 'fifo', or 'mq'.\n");
        terminal_restore_echo();
        if (dev_fd >= 0) {
            close(dev_fd);
        }
        return 1;
    }

    /* Khôi phục terminal và đóng device khi kết thúc */
    terminal_restore_echo();
    if (dev_fd >= 0) {
        close(dev_fd);
        printf("[SENDER] Device closed.\n");
    }

    printf("[SENDER] Sender process terminated cleanly.\n");
    return 0;
}

/*
 * receiver.c - Receiver Application (Task 2)
 *
 * Nhận character/word được gửi từ Sender thông qua kênh IPC (socket, fifo, hoặc mq)
 * và hiển thị ra màn hình terminal.
 */

#include "ipc.h"
#include "socket_ipc.h"
#include "fifo_ipc.h"
#include "mq_ipc.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: ./receiver [socket|fifo|mq]\n");
        return 1;
    }

    printf("========================================================\n");
    printf("   Task 2: Receiver IPC (Awaiting words from Sender)    \n");
    printf("========================================================\n");

    /* Khởi tạo bắt các signal hệ thống (SIGINT, SIGTERM) để tắt chương trình an toàn */
    ipc_init_signals();

    if (strcmp(argv[1], "socket") == 0) {
        socket_receiver();
    }
    else if (strcmp(argv[1], "fifo") == 0) {
        fifo_receiver();
    }
    else if (strcmp(argv[1], "mq") == 0 || strcmp(argv[1], "queue") == 0) {
        mq_receiver();
    }
    else {
        printf("Invalid communication mode! Use 'socket', 'fifo', or 'mq'.\n");
        return 1;
    }

    printf("[RECEIVER] Receiver process terminated cleanly.\n");
    return 0;
}


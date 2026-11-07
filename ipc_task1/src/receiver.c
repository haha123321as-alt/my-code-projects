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

    return 0;
}


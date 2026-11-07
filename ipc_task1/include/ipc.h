#ifndef IPC_H
#define IPC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //de lam gi
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <mqueue.h>
#define SOCKET_PATH "/tmp/ipc_task1_socket"
#define FIFO_PATH "/tmp/ipc_task1_fifo"
#define MQ_NAME "/ipc_task1_mq"
#define BUFFER_SIZE 100
extern volatile sig_atomic_t ipc_running;
void ipc_init_signals(void);
#endif

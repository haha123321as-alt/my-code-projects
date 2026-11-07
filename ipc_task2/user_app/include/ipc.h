#ifndef IPC_H
#define IPC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <mqueue.h>
#include <termios.h>
#include "kbd_dev.h"

/*
 * Đường dẫn IPC endpoints cho Task 2
 */
#define SOCKET_PATH "/tmp/ipc_task2_socket"
#define FIFO_PATH   "/tmp/ipc_task2_fifo"
#define MQ_NAME     "/ipc_task2_mq"
#define BUFFER_SIZE 256

/*
 * Chế độ truyền dữ liệu:
 * - MODE_WORD (0): Thu thập các ký tự thành từ, khi ấn Space (' ') hoặc Enter ('\n') sẽ gửi đi luôn.
 * - MODE_CHAR (1): Gõ phím nào là gửi ngay ký tự đó sang Receiver tức thì (không cần ấn Space hay Enter).
 */
#define MODE_WORD 0
#define MODE_CHAR 1
extern int kbd_send_mode;

/* Biến cờ toàn cục kiểm soát trạng thái chạy của ứng dụng */
extern volatile sig_atomic_t ipc_running;

/* Biến cờ đánh dấu có ký tự mới được gửi từ kernel driver qua signal */
extern volatile sig_atomic_t kbd_event_received;

/* Bộ đệm nhận ký tự an toàn bên trong signal handler */
#define SIG_BUF_SIZE 256
extern volatile char sig_char_buf[SIG_BUF_SIZE];
extern volatile int sig_buf_head;
extern volatile int sig_buf_tail;

/* Khởi tạo xử lý các signal hệ thống (SIGINT, SIGTERM, SIGPIPE) */
void ipc_init_signals(void);

/* Đăng ký signal handler cho SIGUSR1 với SA_SIGINFO để nhận dữ liệu từ driver */
void kbd_init_signal_handler(void);

/* Mở character device driver /dev/keyboard_dev */
int kbd_open_driver(void);

/* Hàm đọc ký tự hoặc từ (word) nhận được từ kernel driver qua signal */
int kbd_get_word(int dev_fd, char *out_buf, size_t max_len);

/* Quản lý terminal raw mode để tránh lặp ký tự (double echo) */
void terminal_disable_echo(void);
void terminal_restore_echo(void);

#endif /* IPC_H */

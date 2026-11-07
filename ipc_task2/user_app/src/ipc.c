/*
 * ipc.c - Quản lý Signal và Giao tiếp Device Driver
 */

#include "ipc.h"

volatile sig_atomic_t ipc_running = 1;
volatile sig_atomic_t kbd_event_received = 0;
int kbd_send_mode = MODE_WORD; /* Mặc định là MODE_WORD (gửi từ khi ấn Space/Enter) */

/* Bộ đệm vòng lưu trữ ký tự được signal handler bắt từ kernel */
volatile char sig_char_buf[SIG_BUF_SIZE];
volatile int sig_buf_head = 0;
volatile int sig_buf_tail = 0;

/* Quản lý cấu hình terminal raw mode */
static struct termios orig_termios;
static int termios_saved = 0;

void terminal_disable_echo(void)
{
    if (isatty(STDIN_FILENO) && !termios_saved) {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
            termios_saved = 1;
            struct termios raw = orig_termios;
            /*
             * Tắt ECHO: Tránh việc terminal tự in ký tự gõ phím, tránh bị lặp (ss, ll)
             * Tắt ICANON: Đọc phím tức thì không cần đợi Enter
             */
            raw.c_lflag &= ~(ECHO | ICANON);
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        }
    }
}

void terminal_restore_echo(void)
{
    if (termios_saved && isatty(STDIN_FILENO)) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        termios_saved = 0;
    }
}

/*
 * Signal handler xử lý khi người dùng ấn Ctrl+C (SIGINT) hoặc kill (SIGTERM)
 */
static void general_signal_handler(int signal)
{
    (void)signal;
    ipc_running = 0;
    terminal_restore_echo();
}

/*
 * =========================================================================
 * 2. Đăng ký signal handler nhận character từ kernel driver qua signal info
 * =========================================================================
 */
static void kbd_sigaction_handler(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)context;

    if (info != NULL) {
        /* Lấy ký tự gửi kèm trong si_value.sival_int */
        char ch = (char)(info->si_value.sival_int & 0xFF);

        /* Lưu ký tự vào bộ đệm vòng an toàn */
        int next = (sig_buf_head + 1) % SIG_BUF_SIZE;
        if (next != sig_buf_tail) {
            sig_char_buf[sig_buf_head] = ch;
            sig_buf_head = next;
        }
        kbd_event_received = 1;
    }
}

void ipc_init_signals(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = general_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    signal(SIGPIPE, SIG_IGN);
}

void kbd_init_signal_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = kbd_sigaction_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIG_KBD, &sa, NULL) < 0) {
        perror("[SENDER] sigaction SIGUSR1");
    } else {
        printf("[SENDER] Registered SIGUSR1 handler with SA_SIGINFO successfully.\n");
    }
}

int kbd_open_driver(void)
{
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("[SENDER] [WARNING] Cannot open %s: %s\n", DEVICE_PATH, strerror(errno));
        printf("[SENDER] [INFO] Please load the kernel module first:\n");
        printf("         cd ../kernel_module && sudo insmod kbd_driver.ko\n");
        printf("[SENDER] [FALLBACK] Will fall back to standard input (stdin) for testing.\n");
        return -1;
    }

    pid_t pid = getpid();
    if (ioctl(fd, KBD_IOCTL_SET_PID, &pid) < 0) {
        printf("[SENDER] Device opened (fd=%d), PID %d auto-registered in driver.\n", fd, pid);
    } else {
        printf("[SENDER] Device %s opened successfully! PID %d registered via IOCTL.\n", DEVICE_PATH, pid);
    }

    return fd;
}

/*
 * Thu thập dữ liệu từ kernel driver qua Signal:
 * - Nếu kbd_send_mode == MODE_CHAR: Gõ phím nào gửi ngay ký tự đó tức thì!
 * - Nếu kbd_send_mode == MODE_WORD: Gõ các phím thành từ, ấn Space (' ') hoặc Enter ('\n') là gửi luôn cả từ!
 */
int kbd_get_word(int dev_fd, char *out_buf, size_t max_len)
{
    if (dev_fd < 0) {
        /* Chế độ Fallback: Nhập từ bàn phím console qua stdin */
        printf("[SENDER] [stdin] : ");
        fflush(stdout);
        if (fgets(out_buf, max_len, stdin) == NULL) {
            return -1;
        }
        size_t len = strlen(out_buf);
        if (len > 0 && out_buf[len - 1] == '\n') {
            out_buf[--len] = '\0';
        }
        return (int)len;
    }

    /* Chế độ nhận phím từ Kernel Driver qua Signal */
    size_t len = 0;
    out_buf[0] = '\0';

    if (kbd_send_mode == MODE_WORD) {
        printf("[SENDER] [Go chu, an SPACE hoac ENTER de gui ngay] -> ");
    } else {
        printf("[SENDER] [Go bat ky phim nao se gui tuc thi sang receiver] -> ");
    }
    fflush(stdout);

    while (ipc_running) {
        /* Nếu trong hàng đợi signal có ký tự */
        if (sig_buf_head != sig_buf_tail) {
            char ch = sig_char_buf[sig_buf_tail];
            sig_buf_tail = (sig_buf_tail + 1) % SIG_BUF_SIZE;

            /* ==============================================================
             * 1. CHẾ ĐỘ CHARACTER MODE: Gửi ngay lập tức mỗi ký tự vừa gõ!
             * ============================================================== */
            if (kbd_send_mode == MODE_CHAR) {
                out_buf[0] = ch;
                out_buf[1] = '\0';
                putchar(ch);
                fflush(stdout);
                return 1; /* Gửi đi ngay lập tức, không cần đợi bất cứ phím nào khác */
            }

            /* ==============================================================
             * 2. CHẾ ĐỘ WORD MODE: Gõ xong từ, ấn SPACE hoặc ENTER để gửi!
             * ============================================================== */
            if (ch == ' ' || ch == '\n') {
                if (len > 0) {
                    out_buf[len] = '\0';
                    printf("\n[SENDER] Captured word: \"%s\" (delim: '%s')\n",
                           out_buf, (ch == ' ') ? "SPACE" : "ENTER");
                    return (int)len;
                }
                continue;
            }

            /* Xử lý phím Backspace ('\b') */
            if (ch == '\b') {
                if (len > 0) {
                    len--;
                    out_buf[len] = '\0';
                    printf("\b \b");
                    fflush(stdout);
                }
                continue;
            }

            /* Thêm ký tự hợp lệ vào từ */
            if (len < max_len - 1) {
                out_buf[len++] = ch;
                out_buf[len] = '\0';
                putchar(ch);
                fflush(stdout);
            } else {
                out_buf[len] = '\0';
                printf("\n[SENDER] Buffer full, sending: \"%s\"\n", out_buf);
                return (int)len;
            }
        } else {
            /* Nghỉ ngắn 5ms để nhường CPU trong lúc đợi signal ngắt bàn phím */
            usleep(5000);
        }
    }

    return -1;
}

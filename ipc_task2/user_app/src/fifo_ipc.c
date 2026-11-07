/*
 * fifo_ipc.c - Truyền thông điệp qua Named Pipe (FIFO)
 *
 * Sender nhận character/word từ keyboard driver qua signal, sau đó ghi vào
 * Named Pipe (/tmp/ipc_task2_fifo) để Receiver đọc và in ra màn hình.
 */

#include "ipc.h"
#include "fifo_ipc.h"

static int write_all(int fd, const char *data, size_t len)
{
    size_t total = 0;
    while (total < len && ipc_running) {
        ssize_t n = write(fd, data + total, len - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
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
 * FIFO SENDER
 * =========================================================================
 */
int fifo_sender(int dev_fd)
{
    char buf[BUFFER_SIZE];
    printf("[SENDER] Waiting for FIFO receiver at %s...\n", FIFO_PATH);

    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd < 0) {
        if (ipc_running) {
            perror("[SENDER] FIFO open");
            printf("[SENDER] Please ensure receiver is running first: ./build/receiver fifo\n");
        }
        return -1;
    }

    printf("[SENDER] FIFO connected!\n");
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

        if (write_all(fd, buf, (size_t)len) < 0) {
            if (ipc_running) perror("[SENDER] FIFO write");
            break;
        }

        if (write_all(fd, "\n", 1) < 0) {
            if (ipc_running) perror("[SENDER] FIFO write newline");
            break;
        }

        printf("[SENDER] -> Sent to receiver via FIFO: \"%s\"\n", buf);
    }

    close(fd);
    printf("\n[SENDER] FIFO disconnected.\n");
    return 0;
}

/*
 * =========================================================================
 * FIFO RECEIVER
 * =========================================================================
 */
int fifo_receiver(void)
{
    char buffer[BUFFER_SIZE];

    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) < 0) {
        perror("[RECEIVER] mkfifo");
        return -1;
    }

    printf("[RECEIVER] FIFO created successfully at %s\n", FIFO_PATH);

    while (ipc_running) {
        printf("[RECEIVER] Waiting for sender to open FIFO...\n");
        fflush(stdout);

        int fd = open(FIFO_PATH, O_RDONLY);
        if (fd < 0) {
            if (errno == EINTR) continue;
            perror("[RECEIVER] FIFO open");
            break;
        }

        printf("[RECEIVER] Sender connected via FIFO!\n");

        size_t pos = 0;
        char ch;
        ssize_t n;

        while (ipc_running) {
            n = read(fd, &ch, 1);
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (n == 0) {
                break; /* Sender đã đóng đầu ghi của FIFO */
            }

            if (ch == '\n') {
                buffer[pos] = '\0';
                printf("[RECEIVER] Received: %s\n", buffer);
                fflush(stdout);
                pos = 0;
            } else if (pos < BUFFER_SIZE - 1) {
                buffer[pos++] = ch;
            } else {
                printf("[RECEIVER] Message too long. Discarding.\n");
                pos = 0;
            }
        }

        close(fd);
        if (pos > 0) {
            buffer[pos] = '\0';
            printf("[RECEIVER] Received: %s\n", buffer);
        }
        printf("[RECEIVER] Sender disconnected.\n");
    }

    unlink(FIFO_PATH);
    printf("[RECEIVER] FIFO receiver stopped.\n");
    return 0;
}


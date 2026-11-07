#include "ipc.h"
#include "fifo_ipc.h"

static int write_all(int fd, const char *data,size_t len)
{
size_t t = 0;
while (t< len && ipc_running ){
ssize_t n = write(fd,data+t, len - t);
if (n <0){
if (errno == EINTR) { continue;} else {return -1;}
}
if(n == 0){
return -1;
} else { t+=n ;} ;
}
return ipc_running ? 0: -1;
}
//==sender==
int fifo_sender(void)
{
    char buf[BUFFER_SIZE];
    printf("[SENDER] Waiting for FF receiver...\n");
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd < 0) {
        if (ipc_running) {perror("FIFO open");} else{  printf("[SENDER] Program stopped.\n");}
        return -1;
    }

    printf("[SENDER] FF connected!\n");

    while (ipc_running) {

        printf("[SENDER] : ");
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            if (!ipc_running || feof(stdin)) { break; }
            if (errno == EINTR) { continue; }
            break;
        }
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') { buf[--len] = '\0'; }
        if (write_all(fd, buf, len) < 0) {
            if (ipc_running) { perror("FIFO write"); }
            break;
        }
        if (write_all(fd, "\n", 1) < 0) {
            if (ipc_running) { perror("FIFO write"); }
            break;
        }
    }
    close(fd);
    printf("\n[SENDER] FF disconnected.\n");
    return 0;
}
//==RECEIVER==
int fifo_receiver(void)
{
    char buffer[BUFFER_SIZE];
    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) < 0) {perror("mkfifo"); return -1;}
    printf("[RECEIVER] FF created successfully.\n");
    while (ipc_running) {
        printf("[RECEIVER] Waiting for sender...\n");
        fflush(stdout);
        int fd = open(FIFO_PATH, O_RDONLY);
        if (fd < 0) {
            if (errno == EINTR) { continue; }
            else { perror("FIFO open"); break; }
        }
printf("[RECEIVER] Sender connected!\n");
size_t pos = 0;
char ch;
ssize_t n;
while (ipc_running) {
n = read(fd, &ch, 1);
if (n < 0) {if (errno == EINTR) {continue;} else{ break;}}
if (n == 0) { break; }
if (ch == '\n') {
buffer[pos] = '\0';
printf("[RECEIVER] Received: %s\n", buffer);
fflush(stdout);
pos = 0;
}
else if (pos < BUFFER_SIZE - 1) {
buffer[pos++] = ch;}
else {printf("[RECEIVER] Message too long. Discarding.\n");
pos = 0;}
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

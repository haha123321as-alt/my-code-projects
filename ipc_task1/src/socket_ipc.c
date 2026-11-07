
#include "ipc.h"
#include "socket_ipc.h"
static int write_all(int fd, const char *data, size_t len)
{
size_t t = 0;
while (t<len && ipc_running){
ssize_t n = write(fd, data+t, len-t);
if(n<0){
if (errno == EINTR)
  {continue;}
  
else {return -1;}
}
if (n==0)
{return -1;} else{ t+=n;}
}
return ipc_running ? 0 : -1;
}
/*
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t count);
File descriptor/ con tror den vung chua du lieu/ so byte muon ghi
tra ve ket qua sobyte ghi duoc/ = 0 khong ghi duoc byte nao/ -1 ghi that bai
tai sao fd lai la int vi trong 1 process kernel cap cho 1 file 1 descriptor co gia tri int
data + total tuong duong &data[0]
EINTR (Interrupted System Call) có nghĩa là system call bị gián đoạn bởi một signal trước khi ghi được dữ liệu.
*/
//==sender==
int socket_sender(void)
{
int s;
struct sockaddr_un addr;//sockaddr_un co gi
char buf[BUFFER_SIZE];
s = socket(AF_UNIX, SOCK_STREAM, 0);
if(s <0){
perror("socket");
return -1 ;
}
memset(&addr, 0,sizeof(addr));
addr.sun_family = AF_UNIX;
strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) -1);
printf("[SENDER] connect...\n");
if (connect(s, (struct sockaddr *)&addr, sizeof(addr))<0){
perror("connect");
close(s);
return -1;
}
printf("[SENDER] Socket connected successfully!\n");
while(ipc_running){
printf("[SENDER] : ");
fflush(stdout);
if(fgets(buf,sizeof(buf),stdin) == NULL){
if (!ipc_running || feof(stdin)) break;
if (errno == EINTR) continue;
break;
}
size_t len = strlen(buf);
if (len>0 && buf[len-1] =='\n'){
buf[--len] = '\0';
}
if (write_all(s,buf,len) <0){
break;
}
if (write_all(s,"\n",1)<0){ break; }
}
close(s);
printf("\n SENDER socket disconnected\n");
return 0;
}
/*
struct sockaddr_un {
    sa_family_t sun_family;//           Xác định loại socket address
    char        sun_path[108];//	Đường dẫn Unix Domain Socket
};
#include <stdio.h>

char *fgets(char *str, int n, FILE *stream);//đọc dữ liệu từ một stream và lưu vào buffer.voi n laSố ký tự tối đa được đọc, tính cả ký tự kết thúc \0

int fflush(FILE *stream); Xử lý buffer (bộ đệm) của stream.stdout luong xuat du lieu, thong thuong prinf se luu giu lieu vao 1 buffer truwoc khi day ra cho nguoi dung ,fflush se yeu cau day ra luon gia tri tra ve 0 l;a thanh cong , EOF: Thất bại.


#include <stdio.h>

int feof(FILE *stream); Stream cần kiểm tra trạng thái EOF, gia tri tra ve 0 chua gap EOF , khac 0 da gawpj EOF
Xóa cờ EOF và error

#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol);// ho dia chi // kieu giao tiep // giao thuc su dung
AF_UNIX	Unix Domain Socket AF_INET IPv4 ,AF_INET6 IPv6
SOCK_STREAM	Giao tiếp dạng luồng byte, thường sử dụng TCP
SOCK_DGRAM	Giao tiếp dạng datagram, thường sử dụng UDP
SOCK_RAW	Giao tiếp raw socket
0: Kernel tự chọn protocol mặc định phù hợp với domain và type.
 tạo ra một endpoint giao tiếp và trả về file descriptor.
 sử dụng file descriptor đó để yêu cầu kết nối đến một socket ở phía bên kia.
#include <sys/types.h>
#include <sys/socket.h>

int connect(int sockfd,
            const struct sockaddr *addr,
            socklen_t addrlen);
ham connect giup thiet lap ket noi den voi 1 dia chi cu the.

Unix Domain Socket (UDS) là một cơ chế giao tiếp giữa các process (Inter-Process Communication - IPC) trên cùng một máy tính.

Nó cho phép hai hoặc nhiều process trao đổi dữ liệu với nhau thông qua socket.
*/
//==receiver==
int socket_receiver(void){
int s;
struct sockaddr_un a;
s = socket(AF_UNIX,SOCK_STREAM,0);
if(s<0){
perror("SOCKET");
return -1;
}
unlink(SOCKET_PATH);
memset(&a, 0, sizeof(a));
a.sun_family = AF_UNIX;
strncpy(a.sun_path, SOCKET_PATH, sizeof(a.sun_path) - 1);//tai sao lai -1
if(bind(s,(struct sockaddr *)&a, sizeof(a))<0){
perror("bind");
close(s);
return -1;
}
if(listen(s,5)<0){
perror("listen");
close(s);
unlink(SOCKET_PATH);
return -1;
}
printf("[RECEIVER] SOCKET SERVER start\n");
while(ipc_running){
int c = accept(s,NULL,NULL);
if (c<0){if(errno == EINTR){continue;} else{perror("accept");break;}}
printf("[RECEIVER]: sender connected\n");
char buf[BUFFER_SIZE];
size_t pos =0;
char ch;
ssize_t n;
while(ipc_running){
n = read(c,&ch,1);
if(n<0){
if (errno == EINTR) {continue;} else {break;}
}
if (n == 0){
break;
}
if (ch == '\n') {
buf[pos] ='\0';
printf("[RECEIVER] Received: %s\n", buf);
fflush(stdout);
pos = 0;
}
else if(pos <BUFFER_SIZE -1){
buf[pos++] = ch;
}
else {
printf("[RECEIVER] Message too long. Discarding.\n");
pos = 0;
}
}
if (pos >0){
buf[pos] = '\0';
printf("[RECEIVER] Received: %s\n", buf);
}
close(c);
printf("[RECEIVER] Sender disconnected.\n");
}
close(s);
unlink(SOCKET_PATH);
printf("[RECEIVER] SOCKET server stopped.\n");
return 0;
}
/*
int unlink(const char *pathname); duong dan den file can xoa, Lệnh trên sẽ xóa filesystem entry /tmp/my_socket.
int bind(int sockfd,// File descriptor cua socket
         const struct sockaddr *addr,// dia chi ma socket su dung
         socklen_t addrlen);// kich thuoc cau truc dia chi

int listen(int sockfd, int backlog);// so luong ket noi dang cho duoc chap nhan toi da theo gioi han hangf doi cua he dieu hanh
sau khi goi bind -> socket da duoc gan voi 1 dia chi, tuy nhien no chua i trang thai cho ket noi tu 1 client .

int accept(int sockfd, // Socket dang o trang thai listen
           struct sockaddr *addr,// dia chi ma client ket noi toi
           socklen_t *addrlen);// con tro bien luu kich thuoc dia chi cua client.
           khi 1 client ket noi toi ham se tra ve 1 socket descriptor moi de giao tiep vowi client do.
           gia tri tra ve >= 0 thanh cong tra ve 1 descriptor moi =-1 that bai

ssize_t read(int fd, void *buf, size_t count);// so byte toi da muon doc
pos lưu vị trí tiếp theo trong buffer mà chúng ta sẽ ghi dữ liệu vào.

*/

#include "ipc.h"
volatile sig_atomic_t ipc_running = 1; //tai sao khong khai bao bien nay la bool vi no co 2 trang thai mat cong khai bao thu vien?
static void signal_handler(int signal){
(void)signal;
ipc_running = 0;
}
void ipc_init_signals(void){
struct sigaction action; 
memset(&action, 0, sizeof(action));
action.sa_handler = signal_handler;// dang ky signal_handle, khi nhan duoc tin hieu se goi ham nay
sigemptyset(&action.sa_mask);//Khởi tạo signal mask rỗng.
action.sa_flags = 0;// khong su dung cac tuy chon bo xung
sigaction(SIGINT,&action,NULL);// nghia la khi nhan duoc SIGINT se goi cau hinh vua tao o tren
sigaction(SIGTERM,&action,NULL);
signal(SIGPIPE, SIG_IGN);
}

/*
sig_atomic_t duoc dinh nghia trong signal.h : cung cap  sig_atomic_t, sigaction(), sigemptyset(), SIGINT, SIGPIPE.
<string.h>: Cung cấp memset().
<signal.h> cũng khai báo signal().
signal_handle chir cung  cap 1 bien de bao cho chuong trinh biet khi nao can dung lai
MUC DICH LA dung chuong trinh co kiem soat: khi ma crt+c terminal se gui SIGINT đến chương trình mac dinh la dung chuong trinh. vi vay chuong trinh can phai xu ly khi nhan bien SIGINT nay
Application Programming Interface
int sigaction(
    int signum,//Signal muốn đăng ký xử lý
    const struct sigaction *act,//	Cấu hình cách xử lý signal
    struct sigaction *oldact//Lưu cấu hình xử lý signal trước đó => NULL khong lay lai yeu cau su ly truoc do
);
// Signal mask ở đây quy định những signal nào sẽ bị block trong lúc handler đang chạy. Mask rỗng nghĩa là không thêm signal nào vào danh sách block này.
SIGPIPE là một signal mà hệ điều hành gửi đến process khi process cố gắng ghi dữ liệu vào một pipe hoặc socket đã bị đóng ở đầu bên kia.
mac dinh khi nhan SIGPIPE thi process se ket thuC
sighandler_t signal(int signum, sighandler_t handler);
signum: Signal muốn đăng ký, ví dụ SIGINT, SIGPIPE.
handler: Con trỏ đến hàm xử lý signal.
Giá trị trả về: Handler cũ trước khi thay đổi.
Signal Ignore
*/

#ifndef KBD_DRIVER_H
#define KBD_DRIVER_H

#include <linux/ioctl.h>

#define DEVICE_NAME "keyboard_dev"
#define CLASS_NAME  "kbd_dev_class"

/*
 * Tín hiệu gửi tới user-space application:
 * Mặc định sử dụng SIGUSR1 (signal số 10 trên x86_64/Linux).
 * Kernel module sẽ gửi signal này kèm theo dữ liệu ký tự qua siginfo_t (si_int).
 */
#define SIG_KBD SIGUSR1

/*
 * Cấu hình IOCTL để ứng dụng user-space có thể giao tiếp bổ sung với driver:
 * - KBD_IOCTL_SET_PID: Đăng ký PID của sender application
 * - KBD_IOCTL_GET_PID: Lấy PID hiện tại đang đăng ký
 * - KBD_IOCTL_CLEAR_BUF: Xóa buffer bàn phím trong kernel
 */
#define KBD_IOC_MAGIC 'k'
#define KBD_IOCTL_SET_PID    _IOW(KBD_IOC_MAGIC, 1, int)
#define KBD_IOCTL_GET_PID    _IOR(KBD_IOC_MAGIC, 2, int)
#define KBD_IOCTL_CLEAR_BUF  _IO(KBD_IOC_MAGIC, 3)

#define KBD_BUFFER_SIZE 256

#endif /* KBD_DRIVER_H */


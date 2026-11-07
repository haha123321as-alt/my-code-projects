#ifndef KBD_DEV_H
#define KBD_DEV_H

#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/keyboard_dev"
#define SIG_KBD SIGUSR1

#define KBD_IOC_MAGIC 'k'
#define KBD_IOCTL_SET_PID    _IOW(KBD_IOC_MAGIC, 1, int)
#define KBD_IOCTL_GET_PID    _IOR(KBD_IOC_MAGIC, 2, int)
#define KBD_IOCTL_CLEAR_BUF  _IO(KBD_IOC_MAGIC, 3)

#endif /* KBD_DEV_H */


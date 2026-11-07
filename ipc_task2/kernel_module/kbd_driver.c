/*
 * kbd_driver.c - Linux Kernel Character Device Driver
 *
 * Nhiệm vụ trong Task 2:
 * 1. Register a character device driver (/dev/keyboard_dev)
 * 2. Register và xử lý keyboard interrupt (IRQ 1 & Keyboard Notifier)
 * 3. Gửi ký tự/từ tới tiến trình Sender ứng dụng qua signal info (send_sig_info với SI_QUEUE)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/keyboard.h>
#include <linux/spinlock.h>
#include <asm/io.h>

#include "kbd_driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Coursework - Task 2");
MODULE_DESCRIPTION("Linux Kernel Module for Keyboard Interrupt Handling and Signal IPC");
MODULE_VERSION("1.0");

/*
 * Cấu trúc quản lý Device Driver:
 * - dev_num: Lưu số hiệu Major & Minor của thiết bị
 * - kbd_cdev: Cấu trúc character device của Linux Kernel
 * - kbd_class: Lớp thiết bị (Device Class) để udev tạo file trong /dev
 * - kbd_device: Đối tượng thiết bị được tạo ra (/dev/keyboard_dev)
 */
static dev_t dev_num;
static struct cdev kbd_cdev;
static struct class *kbd_class = NULL;
static struct device *kbd_device = NULL;

/* PID của tiến trình user-space (Sender) sẽ nhận tín hiệu khi có phím bấm */
static pid_t target_pid = 0;

/* Bộ đệm vòng (Ring Buffer) lưu trữ các ký tự nhận được từ bàn phím */
static char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_head = 0;
static int kbd_tail = 0;
static DEFINE_SPINLOCK(buffer_lock);

/* Trạng thái các phím điều khiển (Shift, CapsLock) */
static int shift_pressed = 0;
static int caps_lock = 0;

/* IRQ 1: Ngắt phần cứng bàn phím tiêu chuẩn (PS/2 / i8042) */
#define KEYBOARD_IRQ 1
static int irq_registered = 0;
static void *irq_dev_id = &target_pid;

/* Notifier block để bắt sự kiện phím qua subsystem console của Linux */
static int notifier_registered = 0;

/*
 * Bảng mã ánh xạ Scancode (Set 1) sang ký tự ASCII chuẩn:
 * Khi người dùng bấm phím, phần cứng gửi một mã scancode (Make Code).
 * Khi nhả phím, scancode có bit cao nhất bật (scancode | 0x80 - Break Code).
 */
static const char scancode_ascii_unshift[128] = {
    [0x01] = 27,   /* ESC */
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', /* Backspace */
    [0x0F] = '\t', /* Tab */
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n', /* Enter */
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';',
    [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x39] = ' ',  /* Space */
};

static const char scancode_ascii_shift[128] = {
    [0x01] = 27,
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')',
    [0x0C] = '_', [0x0D] = '+', [0x0E] = '\b',
    [0x0F] = '\t',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T',
    [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P',
    [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G',
    [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L', [0x27] = ':',
    [0x28] = '"', [0x29] = '~',
    [0x2B] = '|',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B',
    [0x31] = 'N', [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?',
    [0x39] = ' ',
};

/*
 * Hàm lưu ký tự vào Ring Buffer của kernel driver
 */
static void push_to_buffer(char ch)
{
    unsigned long flags;
    spin_lock_irqsave(&buffer_lock, flags);
    kbd_buffer[kbd_head] = ch;
    kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
    if (kbd_head == kbd_tail) {
        /* Nếu buffer đầy, đẩy tail lên để ghi đè ký tự cũ nhất */
        kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    }
    spin_unlock_irqrestore(&buffer_lock, flags);
}

/*
 * =========================================================================
 * 3. Gửi character/word tới sender application qua signal info
 * =========================================================================
 * Sử dụng send_sig_info() kết hợp với struct kernel_siginfo:
 * - info.si_signo: Số hiệu signal (SIGUSR1)
 * - info.si_code: SI_QUEUE (cho phép truyền kèm payload si_int / si_value)
 * - info.si_int: Ký tự bàn phím vừa thu nhận được
 *
 * send_sig_info yêu cầu task_struct của tiến trình đích.
 * Ta tìm task_struct qua pid_task(find_vpid(target_pid), PIDTYPE_PID)
 * được bảo vệ bởi rcu_read_lock() để tránh race condition khi tiến trình kết thúc.
 */
static void send_char_to_user(char ch)
{
    struct kernel_siginfo info;
    struct task_struct *task;
    struct pid *pid_struct;

    if (target_pid <= 0) {
        return;
    }

    /* Đưa ký tự vào buffer nội bộ của driver */
    push_to_buffer(ch);

    /* Chuẩn bị cấu trúc kernel_siginfo để truyền kèm dữ liệu */
    clear_siginfo(&info);
    info.si_signo = SIG_KBD;
    info.si_code = SI_QUEUE;
    info.si_int = (int)(unsigned char)ch; /* Truyền mã ký tự qua si_int */

    rcu_read_lock();
    pid_struct = find_vpid(target_pid);
    if (pid_struct != NULL) {
        task = pid_task(pid_struct, PIDTYPE_PID);
        if (task != NULL) {
            int ret = send_sig_info(SIG_KBD, &info, task);
            if (ret < 0) {
                pr_warn("[kbd_driver] send_sig_info failed with error %d\n", ret);
            } else {
                pr_info("[kbd_driver] Key event: char='%c' (ASCII: %d) -> Sent SIGUSR1 to PID %d\n",
                        (ch >= 32 && ch <= 126) ? ch : '?', (unsigned char)ch, target_pid);
            }
        }
    }
    rcu_read_unlock();
}

/*
 * =========================================================================
 * 2. Đăng ký và xử lý keyboard interrupt
 * =========================================================================
 * Khi có phím bấm hoặc nhả, bộ điều khiển bàn phím (i8042) phát ra IRQ 1.
 * Trình xử lý ngắt (Interrupt Service Routine - ISR) kbd_irq_handler được gọi:
 * - Đọc dữ liệu từ cổng I/O 0x60 bằng lệnh assembly inb(0x60).
 * - Kiểm tra bit 7: Nếu bật (scancode & 0x80) là nhả phím (Break code).
 *                  Nếu tắt (!scancode & 0x80) là bấm phím (Make code).
 * - Cập nhật trạng thái Shift, Caps Lock.
 * - Ánh xạ scancode sang ký tự ASCII và gọi send_char_to_user().
 */
static irqreturn_t kbd_irq_handler(int irq, void *dev_id)
{
    unsigned char scancode;
    char ch = 0;

    (void)irq;
    (void)dev_id;

    /* Đọc scancode từ cổng I/O 0x60 của bàn phím */
    scancode = inb(0x60);

    /* Xử lý bấm/nhả phím Shift */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return IRQ_HANDLED;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        return IRQ_HANDLED;
    }

    /* Xử lý bấm phím Caps Lock (đảo trạng thái) */
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return IRQ_HANDLED;
    }

    /* Bỏ qua nếu là sự kiện nhả phím (Break code có bit 0x80 bật) */
    if (scancode & 0x80) {
        return IRQ_HANDLED;
    }

    /* Ánh xạ sang ký tự ASCII dựa theo trạng thái Shift và Caps Lock */
    if (scancode < sizeof(scancode_ascii_unshift)) {
        int use_shift = shift_pressed;
        char base_ch = scancode_ascii_unshift[scancode];

        /* Nếu là chữ cái a-z và CapsLock bật thì đảo trạng thái Shift */
        if (base_ch >= 'a' && base_ch <= 'z') {
            if (caps_lock) {
                use_shift = !use_shift;
            }
        }

        if (use_shift) {
            ch = scancode_ascii_shift[scancode];
        } else {
            ch = scancode_ascii_unshift[scancode];
        }
    }

    if (ch != 0) {
        send_char_to_user(ch);
    }

    return IRQ_HANDLED;
}

/*
 * Bổ trợ: Keyboard Notifier Callback
 * Trong một số môi trường ảo hóa hoặc kernel mới không cho chia sẻ IRQ 1,
 * Notifier block này bảo đảm vẫn bắt được đầy đủ mọi phím bấm từ console/tty.
 */
static int kbd_notifier_cb(struct notifier_block *nb, unsigned long action, void *data)
{
    struct keyboard_notifier_param *param = (struct keyboard_notifier_param *)data;
    (void)nb;

    /* Chỉ bắt sự kiện phím được ấn xuống (down == 1) */
    if (action == KBD_KEYSYM && param->down) {
        unsigned int val = param->value;
        char ch = 0;

        if (val < 256) {
            ch = (char)val;
        }

        if (ch != 0) {
            send_char_to_user(ch);
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block kbd_nb = {
    .notifier_call = kbd_notifier_cb,
};

/*
 * =========================================================================
 * 1. File Operations của Character Device Driver (/dev/keyboard_dev)
 * =========================================================================
 */

/*
 * dev_open: Được gọi khi user-space gọi open("/dev/keyboard_dev", ...)
 * Tự động đăng ký PID của tiến trình vừa mở device làm target_pid nhận signal!
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
    (void)inodep;
    (void)filep;

    target_pid = current->pid;
    pr_info("[kbd_driver] Device opened. Auto-registered target PID: %d\n", target_pid);
    return 0;
}

/*
 * dev_release: Được gọi khi user-space gọi close(fd)
 * Hủy đăng ký target_pid nếu chính tiến trình đó đóng thiết bị.
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
    (void)inodep;
    (void)filep;

    if (target_pid == current->pid) {
        pr_info("[kbd_driver] Device closed by PID %d. Cleared target_pid.\n", target_pid);
        target_pid = 0;
    }
    return 0;
}

/*
 * dev_read: Cho phép user-space đọc trực tiếp các ký tự/từ đã lưu trong buffer của driver
 */
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    size_t bytes_read = 0;
    unsigned long flags;

    (void)filep;
    (void)offset;

    spin_lock_irqsave(&buffer_lock, flags);
    while (bytes_read < len && kbd_head != kbd_tail) {
        char ch = kbd_buffer[kbd_tail];
        kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
        spin_unlock_irqrestore(&buffer_lock, flags);

        if (copy_to_user(buffer + bytes_read, &ch, 1) != 0) {
            return -EFAULT;
        }
        bytes_read++;

        spin_lock_irqsave(&buffer_lock, flags);
    }
    spin_unlock_irqrestore(&buffer_lock, flags);

    return bytes_read;
}

/*
 * dev_write: Cho phép ghi một chuỗi ký tự/từ vào /dev/keyboard_dev
 * Mỗi ký tự ghi vào sẽ kích hoạt mô phỏng sự kiện bàn phím và gửi signal tới Sender.
 * Tính năng này cực kỳ hữu ích để kiểm thử (echo "hello" > /dev/keyboard_dev).
 */
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    size_t i;
    char *kbuf;

    (void)filep;
    (void)offset;

    if (len == 0) {
        return 0;
    }

    kbuf = kmalloc(len, GFP_KERNEL);
    if (!kbuf) {
        return -ENOMEM;
    }

    if (copy_from_user(kbuf, buffer, len) != 0) {
        kfree(kbuf);
        return -EFAULT;
    }

    for (i = 0; i < len; i++) {
        send_char_to_user(kbuf[i]);
    }

    kfree(kbuf);
    return len;
}

/*
 * dev_ioctl: Hỗ trợ cấu hình bổ sung qua IOCTL từ user application
 */
static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    pid_t new_pid;
    unsigned long flags;

    (void)filep;

    switch (cmd) {
    case KBD_IOCTL_SET_PID:
        if (copy_from_user(&new_pid, (pid_t __user *)arg, sizeof(pid_t))) {
            return -EFAULT;
        }
        target_pid = new_pid;
        pr_info("[kbd_driver] IOCTL set target PID = %d\n", target_pid);
        break;

    case KBD_IOCTL_GET_PID:
        if (copy_to_user((pid_t __user *)arg, &target_pid, sizeof(pid_t))) {
            return -EFAULT;
        }
        break;

    case KBD_IOCTL_CLEAR_BUF:
        spin_lock_irqsave(&buffer_lock, flags);
        kbd_head = 0;
        kbd_tail = 0;
        spin_unlock_irqrestore(&buffer_lock, flags);
        pr_info("[kbd_driver] IOCTL cleared internal buffer\n");
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

/* Bảng các thao tác file của character device driver */
static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .release        = dev_release,
    .read           = dev_read,
    .write          = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

/*
 * =========================================================================
 * Khởi tạo Kernel Module (Module Init)
 * =========================================================================
 */
static int __init kbd_driver_init(void)
{
    int ret;

    pr_info("[kbd_driver] Initializing Keyboard Device Driver Module...\n");

    /* 1. Đăng ký Major và Minor number động cho Character Device */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("[kbd_driver] Failed to allocate char device region (ret=%d)\n", ret);
        return ret;
    }

    /* 2. Khởi tạo và đăng ký cdev */
    cdev_init(&kbd_cdev, &fops);
    kbd_cdev.owner = THIS_MODULE;
    ret = cdev_add(&kbd_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("[kbd_driver] Failed to add cdev (ret=%d)\n", ret);
        goto fail_cdev;
    }

    /* 3. Tạo Device Class trong sysfs */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    kbd_class = class_create(CLASS_NAME);
#else
    kbd_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(kbd_class)) {
        pr_err("[kbd_driver] Failed to create device class\n");
        ret = PTR_ERR(kbd_class);
        goto fail_class;
    }

    /* 4. Tạo Device node (/dev/keyboard_dev) tự động */
    kbd_device = device_create(kbd_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(kbd_device)) {
        pr_err("[kbd_driver] Failed to create device file /dev/%s\n", DEVICE_NAME);
        ret = PTR_ERR(kbd_device);
        goto fail_device;
    }

    pr_info("[kbd_driver] Character device /dev/%s created with Major=%d, Minor=%d\n",
            DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));

    /* 5. Đăng ký Keyboard Interrupt (IRQ 1) */
    ret = request_irq(KEYBOARD_IRQ, kbd_irq_handler, IRQF_SHARED, DEVICE_NAME, irq_dev_id);
    if (ret == 0) {
        irq_registered = 1;
        pr_info("[kbd_driver] Successfully registered IRQ %d handler\n", KEYBOARD_IRQ);
    } else {
        pr_warn("[kbd_driver] Note: request_irq(%d) returned %d (shared IRQ busy or not permitted).\n",
                KEYBOARD_IRQ, ret);
    }

    /* 6. Đăng ký Keyboard Notifier bổ trợ để luôn bắt được phím */
    ret = register_keyboard_notifier(&kbd_nb);
    if (ret == 0) {
        notifier_registered = 1;
        pr_info("[kbd_driver] Successfully registered keyboard notifier callback\n");
    } else {
        pr_warn("[kbd_driver] register_keyboard_notifier returned %d\n", ret);
    }

    pr_info("[kbd_driver] Module loaded successfully!\n");
    return 0;

fail_device:
    class_destroy(kbd_class);
fail_class:
    cdev_del(&kbd_cdev);
fail_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

/*
 * =========================================================================
 * Dọn dẹp Kernel Module (Module Exit)
 * =========================================================================
 */
static void __exit kbd_driver_exit(void)
{
    pr_info("[kbd_driver] Unloading Keyboard Device Driver Module...\n");

    /* Hủy đăng ký keyboard notifier nếu đã đăng ký */
    if (notifier_registered) {
        unregister_keyboard_notifier(&kbd_nb);
        pr_info("[kbd_driver] Unregistered keyboard notifier\n");
    }

    /* Giải phóng IRQ nếu đã đăng ký */
    if (irq_registered) {
        free_irq(KEYBOARD_IRQ, irq_dev_id);
        pr_info("[kbd_driver] Freed IRQ %d\n", KEYBOARD_IRQ);
    }

    /* Xóa Device node /dev/keyboard_dev */
    if (kbd_device) {
        device_destroy(kbd_class, dev_num);
    }

    /* Xóa Device Class */
    if (kbd_class) {
        class_destroy(kbd_class);
    }

    /* Xóa cdev và hủy cấp phát Major/Minor */
    cdev_del(&kbd_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("[kbd_driver] Module unloaded cleanly!\n");
}

module_init(kbd_driver_init);
module_exit(kbd_driver_exit);

























































































































































































































































































































































































































































































































































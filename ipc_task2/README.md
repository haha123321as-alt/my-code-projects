# IPC TASK 2: Kernel Keyboard Driver Intercept & IPC Communication

Dự án này mở rộng từ **Task 1**, kết nối trực tiếp tầng **Linux Kernel Space** với **User Space Application** thông qua **Hardware Interrupt, Signal Info** và các cơ chế **IPC (Socket, FIFO, Message Queue)**.

---

## 1. Yêu Cầu Đã Hoàn Thành

### Kernel Module (`kernel_module/`)
1. **Đăng ký Device Driver**:
   - Khởi tạo và đăng ký Character Device `/dev/keyboard_dev` với dynamic major/minor (`alloc_chrdev_region`, `cdev_init`, `cdev_add`, `class_create`, `device_create`).
   - Cung cấp file operations: `open`, `release`, `read`, `write`, `unlocked_ioctl`.
   - Tự động ghi nhận `target_pid` của tiến trình mở file `/dev/keyboard_dev` (hoặc thông qua IOCTL).
2. **Đăng ký và xử lý Keyboard Interrupt**:
   - Hook ngắt bàn phím IRQ 1 bằng `request_irq(1, kbd_irq_handler, IRQF_SHARED, ...)`.
   - Đọc mã scancode phần cứng qua cổng I/O `0x60` (`inb(0x60)`).
   - Ánh xạ scancode sang ký tự ASCII (hỗ trợ Shift và Caps Lock).
   - Bổ trợ thêm `register_keyboard_notifier` để luôn bắt được phím bấm trong mọi môi trường console/TTY.
3. **Gửi Character/Word tới Sender Application qua Signal Info**:
   - Sử dụng hàm kernel `send_sig_info(SIGUSR1, &info, task)` với cờ `info.si_code = SI_QUEUE`.
   - Dữ liệu ký tự được truyền trực tiếp qua trường `info.si_int = (int)ch`.
   - Quản lý an toàn bằng RCU lock (`rcu_read_lock()` / `rcu_read_unlock()`).

### Application (`user_app/`)
1. **Mở Device Driver**:
   - `kbd_open_driver()` mở `/dev/keyboard_dev` (O_RDWR).
   - Kernel module tự động ghi nhận PID của tiến trình Sender.
2. **Đăng ký Signal Handler & Chuyển tiếp IPC**:
   - Đăng ký handler cho `SIGUSR1` với cấu trúc `sigaction` và cờ `SA_SIGINFO`.
   - `sa_sigaction` nhận trực tiếp `siginfo_t *info`, đọc ký tự từ `info->si_value.sival_int`.
   - Sender thu thập ký tự/từ (word) và chuyển tiếp ngay tới Receiver bằng 1 trong 3 cơ chế IPC giống hệt Task 1:
     - **Unix Domain Socket** (`socket`)
     - **Named Pipe** (`fifo`)
     - **POSIX Message Queue** (`mq`)

---

## 2. Cấu Trúc Thư Mục

```
ipc_task2/
├── Makefile                    # Makefile tổng điều khiển build kernel module & user app
├── README.md                   # Tài liệu hướng dẫn chi tiết
├── kernel_module/
│   ├── Makefile                # Makefile biên dịch kernel module (Kbuild)
│   ├── kbd_driver.c            # Mã nguồn chính của kernel driver
│   └── kbd_driver.h            # Định nghĩa hằng số, IOCTL, tên thiết bị
└── user_app/
    ├── Makefile                # Makefile biên dịch sender và receiver
    ├── include/
    │   ├── ipc.h               # Định nghĩa chung, hàm quản lý signal & device
    │   ├── kbd_dev.h           # Định nghĩa IOCTL chia sẻ với kernel driver
    │   ├── socket_ipc.h        # Khai báo hàm Socket IPC
    │   ├── fifo_ipc.h          # Khai báo hàm FIFO IPC
    │   └── mq_ipc.h            # Khai báo hàm Message Queue IPC
    └── src/
        ├── ipc.c               # Quản lý signal (SIGINT, SIGUSR1 với SA_SIGINFO)
        ├── sender.c            # Chương trình Sender
        ├── receiver.c          # Chương trình Receiver
        ├── socket_ipc.c        # Cài đặt Socket IPC
        ├── fifo_ipc.c          # Cài đặt FIFO IPC
        └── mq_ipc.c            # Cài đặt Message Queue IPC
```

---

## 3. Hướng Dẫn Biên Dịch & Chạy

### Bước 1: Biên dịch toàn bộ
Tại thư mục gốc `/home/dat/ipc_task2`:
```bash
make
```
Lệnh này sẽ tự động biên dịch:
- `kernel_module/kbd_driver.ko`
- `user_app/build/sender`
- `user_app/build/receiver`

### Bước 2: Nạp Kernel Module vào hệ thống
Mở terminal và chạy lệnh:
```bash
cd /home/dat/ipc_task2/kernel_module
sudo make load
```
*(Hoặc `sudo insmod kbd_driver.ko && sudo chmod 666 /dev/keyboard_dev`)*

Kiểm tra kernel log:
```bash
dmesg | tail -n 20
```
Sẽ thấy thông báo device `/dev/keyboard_dev` đã được tạo thành công với Major/Minor number và IRQ 1 đã được đăng ký.

### Bước 3: Chạy Receiver và Sender
Mở 2 cửa sổ terminal riêng biệt:

#### Terminal 1: Chạy Receiver
```bash
cd /home/dat/ipc_task2/user_app
./build/receiver [socket|fifo|mq]
```
Ví dụ:
```bash
./build/receiver socket
```

#### Terminal 2: Chạy Sender
```bash
cd /home/dat/ipc_task2/user_app
./build/sender [socket|fifo|mq]
```
Ví dụ:
```bash
./build/sender socket
```

Khi Sender khởi động, nó tự động mở `/dev/keyboard_dev`, đăng ký PID nhận `SIGUSR1`.
- **Cách 1 (Bàn phím phần cứng)**: Gõ phím trên bàn phím máy tính. Ngắt bàn phím kích hoạt, driver gửi signal `SIGUSR1` kèm mã ký tự tới Sender. Khi gõ xong một từ (ấn phím Space hoặc Enter), Sender gửi từ đó sang Receiver qua IPC!
- **Cách 2 (Mô phỏng ghi trực tiếp vào driver)**:
  Tại một terminal khác:
  ```bash
  echo "hello_world" > /dev/keyboard_dev
  ```
  Driver sẽ mô phỏng sự kiện bàn phím cho từng ký tự và bắn signal tới Sender.
- **Cách 3 (Chế độ Fallback)**:
  Nếu chưa nạp kernel module (chưa có quyền root), Sender tự động chuyển về chế độ đọc từ console (`stdin`) để bạn vẫn kiểm thử đầy đủ luồng IPC Socket/FIFO/MQ.

### Bước 4: Gỡ Kernel Module khi kết thúc
```bash
cd /home/dat/ipc_task2/kernel_module
sudo make unload
```


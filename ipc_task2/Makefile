.PHONY: all user_app kernel_module clean help

all: kernel_module user_app

kernel_module:
	@echo "=== Building Kernel Module ==="
	$(MAKE) -C kernel_module

user_app:
	@echo "=== Building User Applications ==="
	$(MAKE) -C user_app

clean:
	@echo "=== Cleaning Kernel Module ==="
	$(MAKE) -C kernel_module clean
	@echo "=== Cleaning User Applications ==="
	$(MAKE) -C user_app clean

help:
	@echo "Cac lenh build ho tro trong ipc_task2:"
	@echo "  make              - Build ca kernel module va user applications"
	@echo "  make kernel_module - Chi build kernel module (kbd_driver.ko)"
	@echo "  make user_app     - Chi build user app (build/sender, build/receiver)"
	@echo "  make clean        - Don dep build artifacts"


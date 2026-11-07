#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5c3b7c81, "send_sig_info" },
	{ 0xd272d446, "__rcu_read_unlock" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0xd710adbf, "__kmalloc_noprof" },
	{ 0x5cb46e6d, "validate_usercopy_range" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0x9f222e1e, "alloc_chrdev_region" },
	{ 0x0e905811, "cdev_init" },
	{ 0xcd1d5f99, "cdev_add" },
	{ 0xbff2473f, "class_create" },
	{ 0xc866a218, "device_create" },
	{ 0xae5686b7, "class_destroy" },
	{ 0x9126ce86, "request_threaded_irq" },
	{ 0xfd285498, "register_keyboard_notifier" },
	{ 0xa36949ef, "cdev_del" },
	{ 0x0bc5fb0d, "unregister_chrdev_region" },
	{ 0xfd285498, "unregister_keyboard_notifier" },
	{ 0x9dd4105e, "free_irq" },
	{ 0x4bd2df26, "device_destroy" },
	{ 0xd272d446, "__fentry__" },
	{ 0x1349c02f, "const_current_task" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xe8213e80, "_printk" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0x11f4259a, "_raw_spin_lock_irqsave" },
	{ 0x444885a7, "_raw_spin_unlock_irqrestore" },
	{ 0xd272d446, "__rcu_read_lock" },
	{ 0xf8f39583, "find_vpid" },
	{ 0x8230e2d4, "pid_task" },
	{ 0x805f627e, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x5c3b7c81,
	0xd272d446,
	0x90a48d82,
	0xd272d446,
	0x092a35a2,
	0x092a35a2,
	0xd710adbf,
	0x5cb46e6d,
	0xa61fd7aa,
	0xcb8b6ec6,
	0x9f222e1e,
	0x0e905811,
	0xcd1d5f99,
	0xbff2473f,
	0xc866a218,
	0xae5686b7,
	0x9126ce86,
	0xfd285498,
	0xa36949ef,
	0x0bc5fb0d,
	0xfd285498,
	0x9dd4105e,
	0x4bd2df26,
	0xd272d446,
	0x1349c02f,
	0xd272d446,
	0xe8213e80,
	0xbd03ed67,
	0x11f4259a,
	0x444885a7,
	0xd272d446,
	0xf8f39583,
	0x8230e2d4,
	0x805f627e,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"send_sig_info\0"
	"__rcu_read_unlock\0"
	"__ubsan_handle_out_of_bounds\0"
	"__stack_chk_fail\0"
	"_copy_to_user\0"
	"_copy_from_user\0"
	"__kmalloc_noprof\0"
	"validate_usercopy_range\0"
	"__check_object_size\0"
	"kfree\0"
	"alloc_chrdev_region\0"
	"cdev_init\0"
	"cdev_add\0"
	"class_create\0"
	"device_create\0"
	"class_destroy\0"
	"request_threaded_irq\0"
	"register_keyboard_notifier\0"
	"cdev_del\0"
	"unregister_chrdev_region\0"
	"unregister_keyboard_notifier\0"
	"free_irq\0"
	"device_destroy\0"
	"__fentry__\0"
	"const_current_task\0"
	"__x86_return_thunk\0"
	"_printk\0"
	"__ref_stack_chk_guard\0"
	"_raw_spin_lock_irqsave\0"
	"_raw_spin_unlock_irqrestore\0"
	"__rcu_read_lock\0"
	"find_vpid\0"
	"pid_task\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "51770836B69558CB02E6E0F");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Engineer");

unsigned long **sys_call_table;

asmlinkage long (*original_syscall)(void);

asmlinkage long sys_mycall_allocate(size_t size);
asmlinkage long sys_mycall_store(const char __user *data, size_t size);
asmlinkage long sys_mycall_load(char __user *buffer, size_t size);

static char *kernel_memory = NULL;
static size_t memory_size = 0;

unsigned long disable_write_protection(void) {
    unsigned long cr0;
    preempt_disable();
    barrier();
    asm volatile("mov %%cr0, %0" : "=r" (cr0));
    asm volatile("mov %0, %%cr0" : : "r" (cr0 & ~0x00010000));
    barrier();
    preempt_enable();
    return cr0;
}

void enable_write_protection(unsigned long cr0) {
    preempt_disable();
    barrier();
    asm volatile("mov %0, %%cr0" : : "r" (cr0));
    barrier();
    preempt_enable();
}

unsigned long **find_sys_call_table(void) {
    unsigned long sct;

    sct = kallsyms_lookup_name("sys_call_table");
    return (unsigned long **)sct;
}

asmlinkage long sys_mycall_allocate(size_t size) {
    if (size <= 0) {
        return -EINVAL;
    }

    if (kernel_memory) {
        kfree(kernel_memory);
    }

    kernel_memory = kmalloc(size, GFP_KERNEL);
    if (!kernel_memory) {
        return -ENOMEM;
    }

    memory_size = size;
    return 0;
}

asmlinkage long sys_mycall_store(const char __user *data, size_t size) {
    if (size > memory_size) {
        return -EINVAL;
    }

    if (copy_from_user(kernel_memory, data, size)) {
        return -EFAULT;
    }

    return 0;
}

asmlinkage long sys_mycall_load(char __user *buffer, size_t size) {
    if (size > memory_size) {
        return -EINVAL;
    }

    if (copy_to_user(buffer, kernel_memory, size)) {
        return -EFAULT;
    }

    return 0;
}

static int __init my_module_init(void) {
    unsigned long cr0;

    sys_call_table = find_sys_call_table();
    if (!sys_call_table)
        return -1;

    cr0 = disable_write_protection();

    original_syscall = (void *)sys_call_table[__NR_close];
    sys_call_table[__NR_close] = (unsigned long *)sys_mycall_allocate;

    enable_write_protection(cr0);

    printk(KERN_INFO "Module loaded.\n");

    return 0;
}

static void __exit my_module_exit(void) {
    unsigned long cr0;

    cr0 = disable_write_protection();

    sys_call_table[__NR_close] = (unsigned long *)original_syscall;

    enable_write_protection(cr0);

    if (kernel_memory) {
        kfree(kernel_memory);
    }

    printk(KERN_INFO "Module unloaded.\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

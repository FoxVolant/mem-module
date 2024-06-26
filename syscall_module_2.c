#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

unsigned long **sys_call_table;

asmlinkage long (*original_syscall)(void);

asmlinkage long sys_mycall_allocate(size_t size);
asmlinkage long sys_mycall_store(const char __user *data, size_t size);
asmlinkage long sys_mycall_load(char __user *buffer, size_t size);

static char *kernel_memory = NULL;
static size_t memory_size = 0;

unsigned int clear_and_return_cr0(void) {
    unsigned int cr0 = 0;
    unsigned int ret;
    asm volatile("movl %%cr0, %%eax" : "=a"(cr0));
    ret = cr0;
    cr0 &= 0xfffeffff;
    asm volatile("movl %%eax, %%cr0" :: "a"(cr0));
    return ret;
}

void setback_cr0(unsigned int val) {
    asm volatile("movl %%eax, %%cr0" :: "a"(val));
}

static unsigned long **acquire_sys_call_table(void) {
    unsigned long int offset = PAGE_OFFSET;
    unsigned long **sct;

    while (offset < ULLONG_MAX) {
        sct = (unsigned long **)offset;

        if (sct[__NR_close] == (unsigned long *) sys_close)
            return sct;

        offset += sizeof(void *);
    }
    return NULL;
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
    unsigned int cr0;

    sys_call_table = acquire_sys_call_table();
    if (!sys_call_table)
        return -1;

    cr0 = clear_and_return_cr0();

    original_syscall = (void *)sys_call_table[__NR_close];
    sys_call_table[__NR_close] = (unsigned long *)sys_mycall_allocate;

    setback_cr0(cr0);

    printk(KERN_INFO "Module loaded.\n");

    return 0;
}

static void __exit my_module_exit(void) {
    unsigned int cr0;

    cr0 = clear_and_return_cr0();

    sys_call_table[__NR_close] = (unsigned long *)original_syscall;

    setback_cr0(cr0);

    if (kernel_memory) {
        kfree(kernel_memory);
    }

    printk(KERN_INFO "Module unloaded.\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");

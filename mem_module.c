#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static char *kernel_memory = NULL;
static size_t memory_size = 0;

SYSCALL_DEFINE1(allocate_memory, size_t, size)
{
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

SYSCALL_DEFINE2(store_data, const char __user *, data, size_t, size)
{
    if (!kernel_memory || size > memory_size) {
        return -EINVAL;
    }
    if (copy_from_user(kernel_memory, data, size)) {
        return -EFAULT;
    }
    return 0;
}

SYSCALL_DEFINE2(load_data, char __user *, buffer, size_t, size)
{
    if (!kernel_memory || size > memory_size) {
        return -EINVAL;
    }
    if (copy_to_user(buffer, kernel_memory, size)) {
        return -EFAULT;
    }
    return 0;
}

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>       // For struct file_operations
#include <linux/cdev.h>     // For cdev_* functions

#define DEVICE_NAME "mem_device"
#define IOCTL_ALLOCATE_MEMORY _IOW('a', 'a', size_t *)
#define IOCTL_STORE_DATA _IOW('a', 'b', char *)
#define IOCTL_LOAD_DATA _IOR('a', 'c', char *)

static char *kernel_memory;
static size_t memory_size;
static int major;
static struct cdev mem_cdev;

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
        case IOCTL_ALLOCATE_MEMORY:
            if (copy_from_user(&memory_size, (size_t *)arg, sizeof(size_t)))
                return -EFAULT;
            kernel_memory = kmalloc(memory_size, GFP_KERNEL);
            if (!kernel_memory)
                return -ENOMEM;
            break;

        case IOCTL_STORE_DATA:
            if (!kernel_memory)
                return -ENOMEM;
            if (copy_from_user(kernel_memory, (char *)arg, memory_size))
                return -EFAULT;
            break;

        case IOCTL_LOAD_DATA:
            if (!kernel_memory)
                return -ENOMEM;
            if (copy_to_user((char *)arg, kernel_memory, memory_size))
                return -EFAULT;
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = device_ioctl,
    .open = device_open,
    .release = device_release,
};

static int __init mem_module_init(void)
{
    int ret;
    dev_t dev;

    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0)
    {
        pr_err("Failed to allocate character device region\n");
        return ret;
    }

    major = MAJOR(dev);
    cdev_init(&mem_cdev, &fops);
    mem_cdev.owner = THIS_MODULE;
    ret = cdev_add(&mem_cdev, dev, 1);
    if (ret)
    {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    pr_info("Memory module loaded, major number: %d\n", major);
    return 0;
}

static void __exit mem_module_exit(void)
{
    cdev_del(&mem_cdev);
    unregister_chrdev_region(MKDEV(major, 0), 1);
    if (kernel_memory)
        kfree(kernel_memory);
    pr_info("Memory module unloaded\n");
}

module_init(mem_module_init);
module_exit(mem_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Memory Management Module");

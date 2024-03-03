#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include "hello_driver.h"


static ssize_t hello_get_value(struct hello_reg_dev *dev, char *buf, size_t count) {
    if (down_interruptible(&(dev->sem))) {
        return -ERESTARTSYS;
    }
    printk(KERN_INFO "hello_get_value success count = %d\n", dev->length);
    ssize_t size = 0;
    if (count < dev->length) {
        size = -EFAULT;
        goto out;
    }

    if (copy_to_user(buf, dev->data, dev->length)) {
        size = -EFAULT;
        goto out;
    }
    size = dev->length;
    printk(KERN_INFO "hello_get_value success size = %d\n", size);
    out:
    up(&(dev->sem));
    return size;
}

static ssize_t hello_set_value(struct hello_reg_dev *dev, const char *buf, size_t count) {
    if (down_interruptible(&(dev->sem))) {
        return -ERESTARTSYS;
    }
    printk(KERN_INFO "hello_set_value success count = %d\n", count);
    ssize_t size = 0;
    if (count >= VAL_SIZE) {
        size = -EFAULT;
        goto out;
    }
    if (copy_from_user(HELLO_DEV->data, buf, count)) {
        size = -EFAULT;
        goto out;
    }
    size = count;
    HELLO_DEV->length = count;
    printk(KERN_INFO "hello_set_value success size = %d\n", size);
    out:
    up(&(dev->sem));
    return size;
}


static int hello_open(struct inode *inode, struct file *file) {
    struct hello_reg_dev *dev;
    dev = container_of(inode->i_cdev, struct hello_reg_dev, dev);
    file->private_data = dev;
    return 0;
}

static int hello_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t hello_read(struct file *file, char *buf, size_t count, loff_t *f_pos) {
    struct hello_reg_dev *dev = file->private_data;
    return hello_get_value(dev, buf, count);
}

static ssize_t hello_write(struct file *file, const char *buf, size_t count, loff_t *f_pos) {
    struct hello_reg_dev *dev = file->private_data;
    return hello_set_value(dev, buf, count);
}

// 创建 /pro/hello 文件
static int hello_proc_init(void) {
    if (!proc_create(HELLO_DEVICE_PROC_NAME, 0666, NULL, (const struct proc_ops *) &proc_fops)) {
        printk(KERN_ERR "Failed to proc_create \n");
        return -1;
    }
    printk(KERN_INFO "/proc/%s created\n", HELLO_DEVICE_PROC_NAME);
    return 0;
}

// 卸载 /pro/hello 文件
static void hello_proc_exit(void) {
    remove_proc_entry(HELLO_DEVICE_PROC_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", HELLO_DEVICE_PROC_NAME);
}


static ssize_t hello_val_show(struct device *device, struct device_attribute *atr, char *buf) {
    printk(KERN_INFO "hello_val_show \n");
    struct hello_reg_dev *dev = (struct hello_reg_dev *) (device);
    return hello_get_value(dev, buf, PAGE_SIZE);
}

static ssize_t hello_val_store(struct device *device, struct device_attribute *atr, const char *buf, size_t count) {
    printk(KERN_INFO "hello_val_store \n");
    struct hello_reg_dev *dev = (struct hello_reg_dev *) (device);
    return hello_set_value(dev, buf, count);
}

static ssize_t hello_proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos) {
    if (*pos > 0) {
        return 0;
    }
    printk(KERN_INFO "hello_proc_read \n");
    ssize_t length = hello_get_value(HELLO_DEV, usr_buf, count);
    *pos = length;
    return length;
}

//static ssize_t hello_proc_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos) {
//    printk(KERN_INFO "hello_proc_write \n");
//    return hello_set_value(HELLO_DEV, usr_buf, count);
//}


static int hello_val_init(void) {
    HELLO_DEV = kmalloc(sizeof(struct hello_reg_dev), GFP_KERNEL);
    if (!HELLO_DEV) {
        printk(KERN_ERR "Failed to kmalloc \n");
        return -1;
    }

    printk(KERN_INFO "kmalloc success \n");
    memset(HELLO_DEV, 0, sizeof(struct hello_reg_dev));
    // 初始化 信号量
    sema_init(&HELLO_DEV->sem, 1);
    return 0;
}

static void hello_val_exit(void) {
    kfree(HELLO_DEV);
}


static int hello_dev_init(void) {
    // 初始化字符设备
    cdev_init(&(HELLO_DEV->dev), &hello_fops);
    HELLO_DEV->dev.owner = THIS_MODULE;
    HELLO_DEV->dev.ops = &hello_fops;
    // 将字符设备添加到系统中
    if (cdev_add(&(HELLO_DEV->dev), HELLO_DEV->dev_no, 1)) {
        printk(KERN_ERR "Failed to cdev_add \n");
        return -1;
    }
    printk(KERN_INFO "cdev_add success \n");
    return 0;
}

static void hello_dev_exit(void) {
    cdev_del(&HELLO_DEV->dev);
}

static int hello_class_init(void) {
    // 创建设备类型目录  /sys/class/virtual
    struct class *hello_class = class_create(HELLO_DEVICE_CLASS_NAME);
    if (IS_ERR(hello_class)) {
        printk(KERN_ERR "Failed to class_create class\n");
        return -1;
    }
    printk(KERN_INFO "class_create success\n");
    HELLO_DEV->hello_class = hello_class;
    return 0;
}

static void hello_class_exit(void) {
    class_destroy(HELLO_DEV->hello_class);
    printk(KERN_INFO "Device destroyed\n");
}

static int hello_device_init(void) {
    // 创建 /dev/hello 文件
    struct device *hello_device = device_create(HELLO_DEV->hello_class, NULL, HELLO_DEV->dev_no, NULL,
                                                HELLO_DEVICE_DEVICE_NAME);
    if (IS_ERR(hello_device)) {
        printk(KERN_ERR "Failed to device_create\n");
        return -1;
    }
    printk(KERN_INFO "device_create success\n");
    HELLO_DEV->hello_device = hello_device;
    return 0;
}

static void hello_device_exit(void) {
    device_destroy(HELLO_DEV->hello_class, HELLO_DEV->dev_no);
}

static int hello_device_file_init(void) {
    return device_create_file(HELLO_DEV->hello_device, &dev_attr_entry);
}

static void hello_device_file_exit(void) {
    device_remove_file(HELLO_DEV->hello_device, (struct device_attribute *) &(dev_attr_entry.attr));
}


static int hello_alloc_init(void) {
    return alloc_chrdev_region(&HELLO_DEV->dev_no, 0, 1, HELLO_DEVICE_DEVICE_NAME);
}

static void hello_alloc_exit(void) {
    unregister_chrdev_region(HELLO_DEV->dev_no, 1);
}

// 模块加载方法
static int hello_init(void) {
    int ret;
    printk(KERN_INFO "Init hello device\n");
    // 初始化 hello_reg_dev 结构体
    ret = hello_val_init();
    if (ret) {
        goto err_kmalloc;
    }

    // 分配设备编号范围
    ret = hello_alloc_init();
    if (ret) {
        printk(KERN_ERR "Failed to alloc_chrdev_region \n");
        goto err_alloc_chrdev;
    }

    printk(KERN_INFO "alloc_chrdev_region success \n");

    int hello_major = MAJOR(HELLO_DEV->dev_no);
    int hello_minor = MINOR(HELLO_DEV->dev_no);
    printk(KERN_INFO "hello_major %d ,hello_minor %d \n", hello_major, hello_minor);

    // 注册 字符设备
    ret = hello_dev_init();
    if (ret) {
        goto err_cdev_add;
    }

    // 创建 sys/class/virtual/ 目录
    ret = hello_class_init();
    if (ret) {
        goto err_creat_class;
    }

    // 创建 /dev/hello
    ret = hello_device_init();
    if (ret) {
        goto err_create_device;
    }
    //  创建 sys/class/virtual/entry 文件
    ret = hello_device_file_init();
    if (ret) {
        printk(KERN_ERR "Failed to device_create_file \n");
        goto err_create_device_file;
    }
    printk(KERN_INFO "device_create_file success \n");

    // 设置私有数据
    dev_set_drvdata(HELLO_DEV->hello_device, HELLO_DEV);

    // 创建 /proc/hello
    ret = hello_proc_init();
    if (ret) {
        goto err_create_proc_file;
    }

    return 0;
    err_create_proc_file:
    hello_proc_exit();
    err_create_device_file:
    hello_device_file_exit();
    err_create_device:
    hello_device_exit();
    err_creat_class:
    hello_class_exit();
    err_cdev_add:
    hello_dev_exit();
    err_alloc_chrdev:
    hello_alloc_exit();
    err_kmalloc:
    hello_val_exit();
    return ret;
}


// 模块卸载方法
static void hello_exit(void) {
    // 删除 /proc/hello
    hello_proc_exit();
    // 删除 /sys/class/virtual/entry
    hello_device_file_exit();
    // 删除 /dev/hello
    hello_device_exit();
    // 删除 /sys/class/virtual 目录
    hello_class_exit();
    // 从系统中移除字符设备
    hello_dev_exit();
    // 释放分配的设备编号
    hello_alloc_exit();
    // 释放 HELLO_DEV
    hello_val_exit();
}


module_init(hello_init)
module_exit(hello_exit)

MODULE_LICENSE("GPL");

MODULE_AUTHOR("cfeng25");

MODULE_DESCRIPTION("A hello Linux character device driver");

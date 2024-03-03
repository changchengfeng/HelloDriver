//
// Created by cfeng25 on 2/28/24.
//

#ifndef HELLODRIVER_HELLO_DRIVER_H
#define HELLODRIVER_HELLO_DRIVER_H

#include <linux/cdev.h>
#include <linux/semaphore.h>

// 设备名称
#define HELLO_DEVICE_DEVICE_NAME "hello"
// proc文件名
#define HELLO_DEVICE_PROC_NAME "hello"
// 设备 class 类型
#define HELLO_DEVICE_CLASS_NAME "virtual"

#define VAL_SIZE 1024

struct hello_reg_dev {
    char data[VAL_SIZE]; // 能够存储1k个字符
    int length;
    struct semaphore sem; // 信号量 ，用来同步访问 val 字符数组的
    dev_t dev_no;
    struct cdev dev; // 标准的 linux 字符设备结构体变量 ，用来标志该虚拟硬件设备的类型为字符设备
    struct class *hello_class;
    struct device *hello_device;
};

static struct hello_reg_dev *HELLO_DEV = NULL;

static ssize_t hello_get_value(struct hello_reg_dev *dev, char *buf, size_t count);

static ssize_t hello_set_value(struct hello_reg_dev *dev, const char *buf, size_t count);

// 传统设备文件的操作方法
static int hello_open(struct inode *inode, struct file *file);

static int hello_release(struct inode *inode, struct file *file);

static ssize_t hello_read(struct file *file, char *buf, size_t count, loff_t *ppos);

static ssize_t hello_write(struct file *file, const char *buf, size_t count, loff_t *ppos);

// 传统设备文件的操作方法表
static const struct file_operations hello_fops = {
        .owner = THIS_MODULE,
        .open = hello_open,
        .release = hello_release,
        .read = hello_read,
        .write = hello_write,
};


// 文件系统的设备属性操作方法
static ssize_t hello_val_show(struct device *dev, struct device_attribute *atr, char *buf);

static ssize_t hello_val_store(struct device *dev, struct device_attribute *atr, const char *buf, size_t count);

// 文件系统的设备属性
static DEVICE_ATTR(entry, S_IRUGO | S_IWUSR,
                   hello_val_show, hello_val_store);

// proc 文件
static ssize_t hello_proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos);

//static ssize_t hello_proc_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos);

static struct file_operations proc_fops = {
        .owner = THIS_MODULE,
        .read = hello_proc_read,
//        .write = hello_proc_write,
};

static int hello_proc_init(void);

static void hello_proc_exit(void);

static int hello_val_init(void);

static void hello_val_exit(void);

static int hello_dev_init(void);

static void hello_dev_exit(void);

static int hello_class_init(void);

static void hello_class_exit(void);

static int hello_device_init(void);

static void hello_device_exit(void);

static int hello_device_file_init(void);

static void hello_device_file_exit(void);

static int hello_alloc_init(void);

static void hello_alloc_exit(void);

static int hello_init(void);

static void hello_exit(void);

#endif //HELLODRIVER_HELLO_DRIVER_H

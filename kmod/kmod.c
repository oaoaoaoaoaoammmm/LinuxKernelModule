#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <asm/page.h>
#include <linux/pid.h>
#include <asm/pgtable.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <asm/siginfo.h>
#include <linux/rcupdate.h>

#define BUFFER_SIZE 1024
static DEFINE_MUTEX(kmod_mutex);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Danya");
MODULE_VERSION("1.0");

struct net_device_names {
    int size;
    char name[100][30];
};

struct thread_struct_info {
    unsigned long sp;		    /* [GR1 ] kernel stack pointer */
    unsigned long fp;		    /* [GR2 ] kernel frame pointer */
    unsigned long lr;		    /* link register */
    unsigned long pc;	        /* program counter */
    unsigned long sched_lr;	    /* LR from schedule() */
};

static struct dentry *kmod_root;
static struct dentry *kmod_args_file;
static struct dentry *kmod_result_file;

struct net_device_names net_device_names;
struct thread_struct_info thread_struct_info;
int pid = 1;

static ssize_t kmod_args_write(struct file* ptr_file, const char __user* buffer, size_t length, loff_t* ptr_offset) {
    printk(KERN_INFO "kmod: get params\n");
    char kbuf[BUFFER_SIZE];

    if (*ptr_offset > 0 || length > BUFFER_SIZE) {
        return -EFAULT;
    }

    if ( copy_from_user(kbuf, buffer, length) ) {
        return -EFAULT;
    }

    int b = sscanf(kbuf, "%d", &pid);
    printk(KERN_INFO "kmod: pid %d.", pid);
    ssize_t count = strlen(kbuf);
    *ptr_offset = count;
    return count;
}

static int kmod_release(struct inode *inode, struct file *filp) {
    mutex_unlock(&kmod_mutex);
    printk(KERN_INFO "kmod: file is unlocked by module");
    return 0;
}

static void kmod_get_net_device_names(void ) {
    int i = 0;

    struct net_device *net_device;
    net_device = first_net_device(&init_net);

    while (net_device) {
        char *name = net_device->name;
        strcpy(net_device_names.name[i], name);
        printk(KERN_INFO "kmod: net_device found - %s\n", net_device_names.name[i]);
        net_device = next_net_device(net_device);
        i++;
    }

    net_device_names.size = i;
}

static void kmod_get_thread_struct(void ) {

    struct task_struct *task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
    if (task == NULL)
    {
        printk(KERN_ERR "task_struct with pid=%d does not exist\n", pid);
        return;
    }
    struct thread_struct thread = task->thread;

    thread_struct_info.sp = thread.sp;
}

static void set_result(void ) {
    kmod_get_net_device_names();
    kmod_get_thread_struct();
}

static int kmod_read(struct seq_file *sf, void *data) {
    set_result();

    seq_printf(sf, "\nnet_device {\n");
    if (net_device_names.size == 0) {
        seq_printf(sf, "Process hasn't net devices\n");
    } else {
        for (int i = 0; i < net_device_names.size; i++) {
            seq_printf(sf, "net device name is - %s\n", net_device_names.name[i]);
        }
    }

    seq_printf(sf, "}\n");
    seq_printf(sf, "\n");
    seq_printf(sf, "thread_struct {\n");
    seq_printf(sf, "[GR1 ] kernel stack pointer - %lu\n", thread_struct_info.sp);
    seq_printf(sf, "}\n");

    return 0;
}

static int kmod_open(struct inode *inode, struct file *file) {
    mutex_lock(&kmod_mutex);
    printk(KERN_INFO "kmod: file is locked by module");
    return single_open(file, kmod_read, NULL);
}

static struct file_operations kmod_args_ops = {
        .owner   = THIS_MODULE,
        .read    = seq_read,
        .write   = kmod_args_write,
        .open    = kmod_open,
        .release = kmod_release,
};

static int __init kmod_init(void) {
    printk(KERN_INFO "kmod: module loaded =)\n");
    kmod_root = debugfs_create_dir("kmod", NULL);
    kmod_args_file = debugfs_create_file( "kmod_args", 0666, kmod_root, NULL, &kmod_args_ops );
    kmod_result_file = debugfs_create_file( "kmod_result", 0666, kmod_root, NULL, &kmod_args_ops );
    return 0;
}

static void __exit kmod_exit(void) {
    debugfs_remove_recursive(kmod_root);
    printk(KERN_INFO "kmod: module unloaded\n");
    mutex_destroy(&kmod_mutex);
}

module_init(kmod_init);
module_exit(kmod_exit);
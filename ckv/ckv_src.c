#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>


#include "../kv/kv.h"
#include "commands.h"
#include "../kv/buffer_interactions.h"

#define BUFFER_SIZE 1024



char device_buffer[BUFFER_SIZE];
char temp_buffer[BUFFER_SIZE];

static dev_t first;       // Global variable for the first device number
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl;  // Global variable for the device class



static int device_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: open()\n");
  return 0;
}

static int device_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: close()\n");
  return 0;
}


static ssize_t device_read(struct file *fp, char *buff, size_t length, loff_t *ppos)
{
	    int size_in_min_buffer_left, bytes_written, bytes_send;

		size_in_min_buffer_left = MIN(length, BUFFER_SIZE);

		bytes_written = get_values(device_buffer, size_in_min_buffer_left, ppos);

		bytes_send = bytes_written - copy_to_user(buff, device_buffer, bytes_written);
	    *ppos += bytes_send;

        printk(KERN_INFO "charDev : device has been read %d\n", bytes_send);
        return bytes_written;
}


static ssize_t device_write(struct file *fp, const char *buff, size_t length, loff_t *ppos)
{
        int size_in_min_buffer_left, bytes_written, result;

		size_in_min_buffer_left = MIN(length, BUFFER_SIZE - 1);
        bytes_written = size_in_min_buffer_left - copy_from_user(device_buffer, buff, size_in_min_buffer_left);

        device_buffer[bytes_written] = '\0';
        printk(KERN_INFO "charDev : device has been written %d\n", bytes_written);
        *ppos += bytes_written;

        result = make_command(device_buffer, bytes_written);
        if (result < 0)
        {
            printk(KERN_INFO "charDev : make_command returned error: %i\n", result);
        	return result;
        }
        return bytes_written;
}


static struct file_operations pugs_fops =
{
  .owner = THIS_MODULE,
  .open = device_open,
  .release = device_close,
  .read = device_read,
  .write = device_write
};



static int __init ofcd_init(void) /* Constructor */
{
  printk(KERN_INFO "ofcd registered");
  if (alloc_chrdev_region(&first, 0, 1, "Dmitree") < 0)
  {
    return -1;
  }
  
  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  if (device_create(cl, NULL, first, NULL, "ckv") == NULL)
  {
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  cdev_init(&c_dev, &pugs_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  dlm_init();
  //add_test_data();
  return 0;
}
 

static void __exit ofcd_exit(void) /* Destructor */
{
  cdev_del(&c_dev);
  device_destroy(cl, first);
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  printk(KERN_INFO "ofcd unregistered");
}
 
module_init(ofcd_init);
module_exit(ofcd_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmitree Maximenko <https://github.com/Dmitree-Max>");
MODULE_DESCRIPTION("Character device driver for key, values and locks");

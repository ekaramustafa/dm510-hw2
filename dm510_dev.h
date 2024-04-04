#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>	
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
/*include <asm/system.h> */
#include <asm/switch_to.h>
#include <linux/cdev.h>

#define DEVICE_NAME "dm510_dev" /* Dev name as it appears in /proc/devices */
#define MAJOR_NUMBER 255
#define MIN_MINOR_NUMBER 0
#define MAX_MINOR_NUMBER 1

#define DEVICE_COUNT 2

//ioctl
#define GET_BUFFER_SIZE 0
#define SET_BUFFER_SIZE 1
#define GET_MAX_READER 2
#define SET_MAX_READER 3

/* file operations struct */
static struct file_operations dm510_fops = {
	.owner   = THIS_MODULE,
	.read    = dm510_read,
	.write   = dm510_write,
	.open    = dm510_open,
	.release = dm510_release,
    .unlocked_ioctl   = dm510_ioctl
};

static int dm510_open( struct inode*, struct file* );
static int dm510_release( struct inode*, struct file* );
static ssize_t dm510_read( struct file*, char*, size_t, loff_t* );
static ssize_t dm510_write( struct file*, const char*, size_t, loff_t* );
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

struct dm_pipe {
    wait_queue_head_t inq, outq;
    struct buffer *read_buffer;
    struct buffer *write_buffer;
    int nreaders, nwriters;
    struct fasync_struct *async_queue;
    struct mutex mutex;
    struct cdev cdev;
};
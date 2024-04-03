/* Prototype module for second mandatory DM510 assignment */
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
/* Prototypes - this would normally go in a .h file */
#include "buffer.c"
#include <linux/cdev.h>
static int dm510_open( struct inode*, struct file* );
static int dm510_release( struct inode*, struct file* );
static ssize_t dm510_read( struct file*, char*, size_t, loff_t* );
static ssize_t dm510_write( struct file*, const char*, size_t, loff_t* );
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

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
/* end of what really should have been in a .h file */

/* file operations struct */
static struct file_operations dm510_fops = {
	.owner   = THIS_MODULE,
	.read    = dm510_read,
	.write   = dm510_write,
	.open    = dm510_open,
	.release = dm510_release,
        .unlocked_ioctl   = dm510_ioctl
};


dev_t device_devno = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER);

struct dm_pipe {
    wait_queue_head_t inq, outq;
    struct buffer *read_buffer;
    struct buffer *write_buffer;
    int nreaders, nwriters;
    struct fasync_struct *async_queue;
    struct mutex mutex;
    struct cdev cdev;
};

static size_t max_readers = 5;
static struct dm_pipe *dm_pipe_devices;
static struct buffer global_buffers[BUFFER_COUNT];


static void dm_pipe_device_setup(struct dm_pipe *dev, int index){

    int err, devno = device_devno + index;
    cdev_init(&dev->cdev, &dm510_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);

    if(err){
        printk(KERN_NOTICE "Error %d adding dm_pipe %d",err,index);
    }

}

/* called when module is loaded */
int dm510_init_module( void ) {
	/* initialization code belongs here */

	printk(KERN_INFO "DM510: Hello from your device!\n");
    int i, result;

    result = register_chrdev_region(device_devno, DEVICE_COUNT, DEVICE_NAME);
    
    if(result < 0){
        printk(KERN_NOTICE "Unable to register chrdev_region, error %d\n",result);
        return -ENOMEM;
    }

    for(i = 0;i<BUFFER_COUNT;i++){

        global_buffers[i].buffer = kmalloc(BUFFER_SIZE * sizeof(char), GFP_KERNEL);
 
        if(global_buffers[i].buffer == NULL){
            printk(KERN_ERR "Buffer_%d cannot be allocated.", i);
            return -ENOMEM;
        }
        global_buffers[i].wp = global_buffers[i].rp = global_buffers[i].buffer;
	global_buffers[i].size = BUFFER_SIZE;

    }
       
    dm_pipe_devices = kmalloc(DEVICE_COUNT * sizeof(struct dm_pipe), GFP_KERNEL);
    
    if(dm_pipe_devices == NULL){
        unregister_chrdev_region(device_devno, DEVICE_COUNT);
        printk(KERN_ERR "Memory cannot be allocated for dm_devices \n");
        return -ENOMEM;
    }
    
    memset(dm_pipe_devices, 0, DEVICE_COUNT * sizeof(struct dm_pipe));
    
    printk(KERN_INFO "Devices are allocated, being initialized");

    for(int i =0;i< DEVICE_COUNT;i++){
        init_waitqueue_head(&(dm_pipe_devices[i].inq));
        init_waitqueue_head(&(dm_pipe_devices[i].outq));
        mutex_init(&dm_pipe_devices[i].mutex);
        dm_pipe_devices[i].write_buffer = global_buffers + ((i + 1) % BUFFER_COUNT);
        dm_pipe_devices[i].read_buffer = global_buffers + (i % BUFFER_COUNT);
        dm_pipe_device_setup(dm_pipe_devices + i,i);
    }

    return 0;
}

/* Called when module is unloaded */
void dm510_cleanup_module( void ) {

	/* clean up code belongs here */
    int i;

    for(i =0;i< BUFFER_COUNT;i++){
        kfree(global_buffers[i].buffer);
        global_buffers[i].buffer = NULL;
    }

    if(!dm_pipe_devices){
        return;
    }

    for(i=0;i< DEVICE_COUNT;i++){
        cdev_del(&dm_pipe_devices[i].cdev);
    }
    kfree(dm_pipe_devices);
    unregister_chrdev_region(device_devno, DEVICE_COUNT); 
    dm_pipe_devices = NULL;
	printk(KERN_INFO "DM510: Module unloaded.\n");
}


/* Called when a process tries to open the device file */
static int dm510_open( struct inode *inode, struct file *filp ) {
	/* device claiming code belongs here */
    struct dm_pipe *dev; //device info
	printk(KERN_INFO "DM510: Open device!\n");

    dev = container_of(inode->i_cdev, struct dm_pipe, cdev);
    filp->private_data = dev; //for other methods

    if(mutex_lock_interruptible(&dev->mutex)){
        printk(KERN_ERR "Mutex lock is interrupted.\n");
        return -ERESTARTSYS;
    }

    if(filp->f_mode & FMODE_READ){
        if(dev->nreaders >= max_readers){
            mutex_unlock(&dev->mutex);
            printk(KERN_ERR "The maximum number of possible readers reached, limit : %zu", max_readers);
            return -ERESTARTSYS;
        }
        else{
            dev->nreaders++;
        }
    }

    if(filp->f_mode & FMODE_WRITE){
        if(dev->nwriters >= 1){
            mutex_unlock(&dev->mutex);
            printk(KERN_ERR "There is already one device writing.");
            return -ERESTARTSYS;
        }
        else{
            dev->nwriters++;
        }
    }
    mutex_unlock(&dev->mutex);

	return nonseekable_open(inode,filp);
}


/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {
    struct dm_pipe *dev = filp->private_data;
	/* device release code belongs here */
    //remove this filp from the asynchronously notified filp's
    fasync_helper(-1, filp, 0, &dev->async_queue);
    
	mutex_lock(&dev->mutex);
    if(filp->f_mode & FMODE_READ && dev->nreaders){
        dev->nreaders--;
    }
    if(filp->f_mode & FMODE_WRITE && dev->nwriters){
        dev->nwriters--;
    }
    mutex_unlock(&dev->mutex);

	return 0;
}


/* Called when a process, which already opened the dev file, attempts to read from it. */
static ssize_t dm510_read( struct file *filp,
    char *buf,      /* The buffer to fill with data     */
    size_t count,   /* The max number of bytes to read  */
    loff_t *f_pos )  /* The offset in the file           */
{
	
    struct dm_pipe *dev = filp->private_data;

    if(mutex_lock_interruptible(&dev->mutex)){
        printk(KERN_ERR "Mutex lock is interrupted\n");
        return -ERESTARTSYS;
    }

    while(dev->read_buffer->rp == dev->read_buffer->wp){

        mutex_unlock(&dev->mutex);
        if(filp->f_flags & O_NONBLOCK){
            printk(KERN_ERR "File-pointer blocked");
            return -EAGAIN;
        }
        if(wait_event_interruptible(dev->inq, (dev->read_buffer->rp != dev->read_buffer->wp))){
            printk(KERN_ERR "Reader is interrupted\n");
            return -ERESTARTSYS;
        }
        if(mutex_lock_interruptible(&dev->mutex)){
            printk(KERN_ERR "Mutex lock is interrupted\n");
            return -ERESTARTSYS;
        }

    }

    count = buffer_read(dev->read_buffer,buf,count); 
    mutex_unlock(&dev->mutex);
    //wake_up_interruptible(&dev->outq);
    int i;
    for(i = 0 ; i < DEVICE_COUNT ; i++){
    	wake_up_interruptible(&dm_pipe_devices[i].outq);
    }
	/* read code belongs here */
	return count; 
}

static int spacefree(struct buffer *buff){
    if(buff->rp == buff->wp){
     return buff->size-1;  
    }
    return ((buff->rp + buff->size - buff->wp) % buff->size) - 1;
}


/* Called when a process writes to dev file */
static ssize_t dm510_write( struct file *filp,
    const char *buf,/* The buffer to get data from      */
    size_t count,   /* The max number of bytes to write */
    loff_t *f_pos )  /* The offset in the file           */
{

    struct dm_pipe *dev = filp->private_data;
    if(mutex_lock_interruptible(&dev->mutex)){
        printk(KERN_ERR "Mutex lock is interrupted\n");
        return -ERESTARTSYS;
    }
    if(count > global_buffers->size){
        printk(KERN_ERR "Message is too long for the buffer\n");
        return -EMSGSIZE;
    }

    while(spacefree(dev->write_buffer) < count){
        
        mutex_unlock(&dev->mutex);
        if(filp->f_flags & O_NONBLOCK){
            printk(KERN_ERR "File-pointer is blocked\n");
            return -EAGAIN;
        }
        if(wait_event_interruptible(dev->outq,
                    (spacefree(dev->write_buffer) >= count))){
            printk(KERN_ERR "Writer is interrupted\n");
            return -EAGAIN;
        }

        if(mutex_lock_interruptible(&dev->mutex)){
            printk(KERN_ERR "Mutex lock is interrupted\n");
            return -ERESTARTSYS;
        }
    }

    count = buffer_write(dev->write_buffer,(char *)buf,count);
    int i;
    for(i = 0 ; i < DEVICE_COUNT ; i++){
    	wake_up_interruptible(&dm_pipe_devices[i].inq);
    }


    if(dev->async_queue){
       kill_fasync(&dev->async_queue, SIGIO, POLL_IN); 
    }
    
    mutex_unlock(&dev->mutex);
	return count; //return number of bytes written
}

/* called by system call icotl */ 
long dm510_ioctl( 
    struct file *filp, 
    unsigned int cmd,   /* command passed from the user */
    unsigned long arg ) /* argument of the command */
{
	/* ioctl code belongs here */
	printk(KERN_INFO "DM510: ioctl called.\n");
    switch(cmd){
        case GET_BUFFER_SIZE:
            return global_buffers->size;
        
       case SET_BUFFER_SIZE:
            unsigned long new_size = arg;
            if (new_size == 0 || new_size > BUFFER_SIZE){
                printk(KERN_ERR "Invalid Buffer Size is provided %lu \n", new_size);
                }
                return -EINVAL; // Invalid buffer size
            for (int i = 0; i < BUFFER_COUNT; i++) {
                int used_space = global_buffers[i].size - spacefree(&global_buffers[i]);
                if (used_space > new_size) {
                    printk(KERN_ERR "Buffer(%d) has %d amount of used space, cannot be reduced to size %lu.\n", i, used_space, new_size);
                    return -EINVAL; // Cannot reduce buffer size
                }
            }
            for (int i = 0; i < BUFFER_COUNT; i++) {
            	int result = buffer_resize(&global_buffers[i],new_size);
            	if(result !=0){
            		return result;
            	}
            }
            break;

       case GET_MAX_READER:
            return max_readers;
        
       case SET_MAX_READER:
            max_readers = arg;
            break;
	}
	return 0; //has to be changed
}

module_init( dm510_init_module ); //this call is mandatory
module_exit( dm510_cleanup_module ); //this call is mandatory 

MODULE_AUTHOR( "...Ebubekir Karamustafa." );
MODULE_LICENSE( "GPL" );
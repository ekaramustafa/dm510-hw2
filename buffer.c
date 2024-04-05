#include "buffer.h"


size_t buffer_read(struct buffer *buf, char *seq, size_t size){
    size_t bytes_read = 0;
    size_t available_data;

    if (buf->rp < buf->wp)
        available_data = buf->wp - buf->rp;
    else
        available_data = (buf->buffer + buf->size) - buf->rp;

    size_t to_read = min(available_data, size);
    if (to_read > 0) {
        if(copy_to_user(seq, buf->rp, to_read) != 0){
            return -EFAULT;
        }

        buf->rp += to_read;
        bytes_read += to_read;

        if (buf->rp == buf->buffer + buf->size)
            buf->rp = buf->buffer;
    }

    return bytes_read;
}


size_t buffer_write(struct buffer *buf, char *seq, size_t size){
    size_t bytes_written = 0;

    // Calculate the available space in the buffer
    size_t available_space;
    if (buf->wp < buf->rp)
        available_space = buf->rp - buf->wp - 1;
    else
        available_space = (buf->buffer + buf->size) - buf->wp;

    // Determine the amount to write
    size_t to_write = min(available_space, size);

    if (to_write > 0) {
        if(copy_from_user(buf->wp, seq, to_write) != 0){
        	return -EFAULT;
        }

        buf->wp += to_write;
        bytes_written += to_write;

        if (buf->wp == buf->buffer + buf->size)
            buf->wp = buf->buffer;
    }

    return bytes_written;
}


int buffer_resize(struct buffer *buf, size_t size) {
    void *new_buffer = kmalloc(size, GFP_KERNEL);

    if (!new_buffer) {
        printk(KERN_ERR "Error in memory allocation while resizing the buffer\n");
        return -ENOMEM; // Return error code if memory allocation fails
    }

    if (buf->rp <= buf->wp) {
        size_t data_size = buf->wp - buf->rp;
        memcpy(new_buffer, buf->rp, data_size);
        buf->rp = new_buffer;
        buf->wp = new_buffer + data_size;
    } else {
        size_t data_size_1 = buf->size - (buf->rp - buf->buffer);
        size_t data_size_2 = buf->wp - buf->buffer;
        
        memcpy(new_buffer, buf->rp, data_size_1);
        memcpy(new_buffer + data_size_1, buf->buffer, data_size_2);

        buf->rp = new_buffer;
        buf->wp = new_buffer + data_size_1 + data_size_2;
    }

    kfree(buf->buffer); // Free old buffer after data has been copied
    buf->buffer = new_buffer;
    buf->size = size;

    return 0; // Return success
}
#ifndef DM510_BUFFER_H
#define DM510_BUFFER_H

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#define BUFFER_COUNT 2
#define BUFFER_SIZE 4000

struct buffer{
    char *buffer;
    size_t size;
    char *rp, *wp;
    //struct mutex mutex;
};

size_t buffer_read(struct buffer *buf, char * seq, size_t size);
size_t buffer_write(struct buffer *buf, char * seq, size_t size);
int buffer_resize(struct buffer *buf, size_t size);

#endif /* DM510_BUFFER_H */

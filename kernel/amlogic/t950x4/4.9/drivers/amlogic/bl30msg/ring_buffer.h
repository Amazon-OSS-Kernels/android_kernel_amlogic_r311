#ifndef __MESON_RING_BUFFER_H__
#define __MESON_RING_BUFFER_H__
#include <linux/types.h>

struct ring_buffer {
	unsigned int magic;      // magic number
	unsigned int lock;       // exclusive lock (implement future)
	unsigned int size;       // total size of ring buffer data
	unsigned int head;       // head offset, kernel move it
	unsigned int tail;       // tail offset, M3 move it
	unsigned int len;        // available log data in ring buffer
	char data[4];            // log buffer payload start
};

#define BL30MSG_LEN 100
#define BL30MSG_MAGIC 0xABC123

#define BL30MSG_BUF_BASE 0xFFFDD000
#define BL30MSG_BUF_SIZE 0x1000
#define BL30MSG_BUF_OFFSET 100

struct ring_buffer *init_ring_buffer(unsigned char *buffer, unsigned int size);
unsigned int read_remove_ring_buffer(struct ring_buffer *rb,
	unsigned char *buffer, unsigned int size);
unsigned int read_keep_ring_buffer(struct ring_buffer *rb, unsigned char *buffer);
unsigned int write_ring_buffer_string(struct ring_buffer *rb,
	unsigned char *buffer, unsigned int size);
unsigned int write_ring_buffer_char(struct ring_buffer *rb, unsigned char c);
#endif


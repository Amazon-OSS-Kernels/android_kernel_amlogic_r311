/*
 *  Ring Buffer for BL301 log share to kernel
 *
 *  Copyright (C) 2019-2020 amazon
 *
 */

#define BL30MSG_BUF_BASE 0xFFFDD000
#define BL30MSG_BUF_SIZE 0x1000
#define BL30MSG_LEN 100
 
#define MAGIC  0x11223344

#ifndef NULL
#define NULL   ((void *)0)
#endif

struct ring_buffer {
	unsigned int magic;      // magic number
	unsigned int lock;       // exclusive lock (implement future)
	unsigned int size;       // total size of ring buffer data
	unsigned int head;       // head offset, kernel move it
	unsigned int tail;       // tail offset, M3 move it
	unsigned int len;        // available log data in ring buffer
	char data[4];            // log buffer payload start
};

extern unsigned int write_ring_buffer(unsigned char *buffer, unsigned int size);

/*
 *  Ring Buffer for BL301 log share to kernel
 *
 *  Copyright (C) 2019-2020 amazon
 *
 */

#include "ring_buffer.h"



struct ring_buffer *rb=(struct ring_buffer *)BL30MSG_BUF_BASE;

unsigned int write_ring_buffer(unsigned char *buffer, unsigned int size)
{
	unsigned int count,wlen;

	if(!rb || rb->magic != MAGIC)
		return 0;

	count = 0;
	wlen = size;
	while(wlen--){
		rb->data[rb->tail++] = buffer[count++];
		rb->tail %= rb->size;
		rb->len++;
	}

	/* New data will overwrite the old data */
	if (rb->len > rb->size){
		rb->head = rb->tail;
		rb->len = rb->size;
	}

	return count;
}

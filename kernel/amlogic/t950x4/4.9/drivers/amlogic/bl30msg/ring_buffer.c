/* SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 *
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/slab.h>
#include "ring_buffer.h"

struct ring_buffer *init_ring_buffer(unsigned char *buffer, unsigned int size)
{
	struct ring_buffer *b = (struct ring_buffer *)buffer;

	if (!b || size < sizeof(struct ring_buffer))
		return NULL;
	if (b->magic == BL30MSG_MAGIC) {
		pr_warn("rbuf:magic is ok\n");
		return b;
	}
	pr_warn("rbuf:magic is not writen[0x%x]\n", b->magic);

	b->magic = BL30MSG_MAGIC;
	b->size = size - sizeof(struct ring_buffer) + 4;
	b->head = 0;
	b->tail = 0;
	b->len  = 0;
	return b;
}

unsigned int read_revome_ring_buffer(struct ring_buffer *rb,
	unsigned char *buffer, unsigned int size)
{
	unsigned int count, rlen;

	if (!rb || rb->len == 0)
		return 0;

	count = 0;
	rlen = size < rb->len ? size : rb->len;

	while (rlen--) {
		buffer[count++] = rb->data[rb->head++];
		rb->head %= rb->size;
		rb->len--;
	}

	return count;
}

unsigned int read_keep_ring_buffer(struct ring_buffer *rb, unsigned char *buffer)
{
	unsigned int count, rlen, head;

	if (!rb || rb->len == 0)
		return 0;

	count = 0;
	rlen = rb->len;
	head = rb->head;

	while (rlen--) {
		buffer[count++] = rb->data[head++];
		head %= rb->size;
	}
	return count;
}

unsigned int write_ring_buffer_string(struct ring_buffer *rb,
	unsigned char *buffer, unsigned int size)
{
	unsigned int count, wlen;

	if (!rb || rb->magic != BL30MSG_MAGIC)
		return 0;

	count = 0;
	wlen = size;
	while (wlen--) {
		rb->data[rb->tail++] = buffer[count++];
		rb->tail %= rb->size;
		rb->len++;
	}

	/* New data will overwrite the old data */
	if (rb->len > rb->size) {
		rb->head = rb->tail;
		rb->len = rb->size;
	}

	return count;
}

unsigned int write_ring_buffer_char(struct ring_buffer *rb, unsigned char c)
{
	if (!rb || rb->magic != BL30MSG_MAGIC)
		return 0;

	rb->data[rb->tail++] = c;
	rb->tail %= rb->size;
	rb->len++;

	/* New data will overwrite the old data */
	if (rb->len > rb->size) {
		rb->head = rb->tail;
		rb->len = rb->size;
	}
	return 1;
}


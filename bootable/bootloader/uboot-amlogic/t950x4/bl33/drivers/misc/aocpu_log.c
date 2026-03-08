// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * common/cmd_aocpu_log.c
 *
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 *
 */

#include <config.h>
#include <common.h>
#include <command.h>

#define BL30MSG_BUF_BASE 0xFFFC7800
#define BL30MSG_BUF_SIZE 0x800
#define BL30MSG_MAGIC  0x11223344

typedef struct amp_lock_s {
	volatile int turn;
	volatile int req[2];
} amp_lock;
struct ring_buffer {
	unsigned int magic;      // magic number
	//tSoftAmpLock lock;     // exclusive lock (implement future)
	amp_lock lock;           // exclusive lock (implement future)
	unsigned int size;       // total size of ring buffer data
	unsigned int head;       // for read data offset, arm(kernel) maintain, kernel move it
	unsigned int tail;       // for write data offset, risc-v maintain it
	unsigned int len;        // available log data in ring buffer
	char data[4];            // log buffer payload start
};

/* log type */
#define UBOOT_LOG 0
#define AOCPU_LOG 1

#define LOG_BUF_SIZE     4096
static char log_buf_aocpu[LOG_BUF_SIZE];

#define LOCK 1
#define RISC_V_CPU    0
#define ARM_CPU       1

static void Lock_EnterCritical(void)
{
    //local_irq_disable();
}

static void Lock_ExitCritical(void)
{
    //local_irq_enable();
}

static int amp_lock_obtain(amp_lock *lock, int self)
{
#if LOCK
	int other = 1 - self;

    // disable interrupt here
	Lock_EnterCritical();

	lock->req[self] = 1;
	if (lock->req[other]) {
		lock->req[self] = 0;
		while (lock->turn != self)
			;
		lock->req[self] = 1;
		while (lock->req[other])
			;
	}
#endif
	return 0;
}

static int amp_lock_release(amp_lock *lock, int self)
{
#if LOCK
	int other;

	other = 1 - self;
	lock->turn = other;
	lock->req[self] = 0;

    // enable interrupt here
	Lock_ExitCritical();
#endif
	return 0;
}

char *aocpu_log_dump(int *buf_size)
{
	struct ring_buffer *rb;
	int buf_len = 0;
	//int rb_head = 0;
	int count = 0;

	rb = (struct ring_buffer *)BL30MSG_BUF_BASE;
	if (!rb || rb->magic != BL30MSG_MAGIC) {
		printf("ring buffer is NULL or MAGIC value is error\n");
		return NULL;
	}

	if (!rb->len || rb->len > BL30MSG_BUF_SIZE || rb->size > BL30MSG_BUF_SIZE) {
		printf("ring buffer size is error\n");
		return NULL;
	}

	amp_lock_obtain(&rb->lock, ARM_CPU);
	*buf_size = rb->len;
	buf_len = rb->len < LOG_BUF_SIZE ? rb->len : LOG_BUF_SIZE;
	//rb_head = rb->head;
	while (buf_len--) {
		//log_buf_aocpu[count++] = rb->data[rb_head++];
		//rb_head %= rb->size;
		log_buf_aocpu[count++] = rb->data[rb->head++];
		rb->head %= rb->size;
		rb->len--;
	}
	amp_lock_release(&rb->lock, ARM_CPU);

	log_buf_aocpu[count] = '\0';

	return log_buf_aocpu;
}

void aocpu_log_external_print(void)
{
	int log_size = 0;
	char *log_buf = NULL;
	int pos = 0, cnt = 0, count = 0;
	char str[1024];/*printk only can print 2048bytes*/

	log_buf = aocpu_log_dump(&log_size);

	cnt = log_size / 1024;
	count = cnt * 1024;

	printf("log_size=%d\n", log_size);
	while (pos < count && cnt) {
		memcpy(str, log_buf + pos, 1023);
		pos += 1023;
		cnt--;
		str[1023] = '\0';
		printf("%s", str);
	}

	if (log_size - pos > 0) {
		memcpy(str, log_buf + pos, log_size - pos);
		str[log_size - pos] = '\0';
		printf("%s\n", str);
	}
}

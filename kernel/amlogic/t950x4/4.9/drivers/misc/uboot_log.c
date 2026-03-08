/*
 * uboot log driver.
 *
 * This module use to export uboot log to user space
 *
 * Copyright (C) 2019-2020 amazon
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#define pr_fmt(fmt)	"uboot_log: " fmt
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mm_types.h>
#include <linux/memblock.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/suspend.h>
#include <linux/notifier.h>

#include <linux/of.h>
#include <linux/of_reserved_mem.h>

#define ULOG_COOKIE    0x474f4c55 /* "ULOG" in ASCII */
struct uboot_log {
	struct uboot_log_header {
		unsigned int cookie;
		unsigned int max_size;
		unsigned int size_written;
		unsigned int idx;
	} header;
	char data[0];
};

static phys_addr_t log_paddr;
static unsigned long log_size;
static void *log_vaddr;

/*aocpu log data structure*/
#define BL30MSG_BUF_BASE 0xFFFC7800
#define BL30MSG_BUF_SIZE 0x800
#define BL30MSG_MAGIC  0x11223344
typedef struct amp_lock_s {
	volatile int turn;
	volatile int req[2];
} amp_lock;
#if 1
struct ring_buffer {
	unsigned int magic;      // magic number
	amp_lock     lock;       // exclusive lock (implement future)
	unsigned int size;       // total size of ring buffer data
	unsigned int head;       // head offset, kernel move it
	unsigned int tail;       // tail offset, AOCPU move it
	unsigned int len;        // available log data in ring buffer
	char data[4];            // log buffer payload start
};
#endif
#if 0
struct ring_buffer {
	unsigned int magic;      // magic number
	//tSoftAmpLock lock;       // exclusive lock (implement future)
	amp_lock     lock;           // exclusive lock (implement future)
	unsigned int size;       // total size of ring buffer data
	unsigned int head;       // for read data offset, arm(kernel) maintain, kernel move it
	unsigned int tail;       // for write data offset, risc-v maintain it
	unsigned int last_head;  // for record head offset, risc-v maintain, it can know if kernel is read data
	unsigned int full_flag;  // mark buf is full, risc-v maintain
	unsigned int init_flag;  //0: no start write data, 1: wrote data to buf
	char data[4];            // log buffer payload start
};
#endif

/* log type */
#define UBOOT_LOG 0
#define AOCPU_LOG 1
#define LOG_BUF_SIZE     4096
static char log_buf_aocpu[LOG_BUF_SIZE];

#define LOCK 1
#define RISC_V_CPU    0
#define ARM_CPU       1

char *uboot_log_dump(int *size);
char *aocpu_log_dump(int *buf_size);

static void Lock_EnterCritical(void)
{
    local_irq_disable();
}

static void Lock_ExitCritical(void)
{
    local_irq_enable();
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
	int other = 1 - self;
	lock->turn = other;
	lock->req[self] = 0;

    // enable interrupt here
	Lock_ExitCritical();
#endif
	return 0;
}


void log_external_print(int log_type)
{
	int log_size = 0;
	char *log_buf = NULL;
	int pos = 0, cnt = 0, count = 0;
	char str[1024];/*printk only can print 2048bytes*/

	if (log_type == UBOOT_LOG)
		log_buf = uboot_log_dump(&log_size);
	else if (log_type == AOCPU_LOG)
		log_buf = aocpu_log_dump(&log_size);

	cnt = log_size / 1024;
	count = cnt * 1024;
	pr_info("log_type = %d\n", log_type);

	pr_info("log_size=%d\n", log_size);
	while (pos < count && cnt) {
		memcpy(str, log_buf + pos, 1023);
		pos += 1023;
		cnt--;
		str[1023] = '\0';
		printk("%s", str);
	}

	if (log_size - pos > 0) {
		memcpy(str, log_buf + pos, log_size - pos);
		str[log_size - pos] = '\0';
		printk("%s\n", str);
    }
}
EXPORT_SYMBOL(log_external_print);

static int mmap_uboot_log(void)
{
	struct page **pages;
	phys_addr_t page_start;
	unsigned int npages;
	pgprot_t prot;
	unsigned int i;
	phys_addr_t start = log_paddr;
	unsigned long size = log_size;

	pr_info("log_paddr   %p\n", (void *)log_paddr);
	/* mmap bl_log header */
	page_start = start - offset_in_page(start);
	npages = DIV_ROUND_UP(size + offset_in_page(start), PAGE_SIZE);

	prot = pgprot_noncached(PAGE_KERNEL);
	pages = vmalloc(sizeof(struct page *) * npages);
	if (!pages)
		return -ENOMEM;

	for (i = 0; i < npages; i++) {
		phys_addr_t addr;

		addr = page_start + i * PAGE_SIZE;
		pages[i] = pfn_to_page(addr >> PAGE_SHIFT);
	}
	log_vaddr = vmap(pages, npages, VM_MAP, prot);
	pr_info("log_vaddr   %p\n", log_vaddr);
	vfree(pages);
	if (!log_vaddr) {
		pr_err("%s: Failed to map %u pages\n", __func__, npages);
		return -ENOMEM;
	}

	return 0;
}

char *uboot_log_dump(int *size)
{
	unsigned int log_len;
	struct uboot_log *uboot_log_hr;
	int ret = 0;

	if (!log_vaddr)
		ret = mmap_uboot_log();

	if (ret)
		return NULL;

	uboot_log_hr = (struct uboot_log *)((char *)log_vaddr + offset_in_page(log_paddr));
	if (ULOG_COOKIE == uboot_log_hr->header.cookie) {
		log_len = min(uboot_log_hr->header.size_written, uboot_log_hr->header.max_size);
		uboot_log_hr->data[log_len] = '\0';
		*size = log_len;
		return uboot_log_hr->data;
	} else
		pr_err("magic number 0x%x\n", uboot_log_hr->header.cookie);

	return NULL;
}
EXPORT_SYMBOL(uboot_log_dump);

static int uboot_log_show(struct seq_file *m, void *v)
{
	int log_len = 0;
	char *uboot_log = uboot_log_dump(&log_len);

	if (uboot_log)
		seq_write(m, uboot_log, log_len);
	else
		seq_printf(m, "error !");

	return 0;
}

static int uboot_log_file_open(struct inode *inode, struct file *file)
{
	return single_open(file, uboot_log_show, inode->i_private);
}

static const struct file_operations uboot_log_file_ops = {
	.owner = THIS_MODULE,
	.open = uboot_log_file_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

char *aocpu_log_dump(int *buf_size)
{
	unsigned char __iomem *bl30_buf_base_addr;
	struct ring_buffer *rb;
	int buf_len = 0;
	//int rb_head = 0;
	int count = 0;

	bl30_buf_base_addr = ioremap(BL30MSG_BUF_BASE, BL30MSG_BUF_SIZE);
	if (!bl30_buf_base_addr) {
		pr_err("%s: failed to map 0x%x size at 0x%x\n",
				__func__, BL30MSG_BUF_BASE, BL30MSG_BUF_SIZE);
		return NULL;
	}

	rb = (struct ring_buffer *)bl30_buf_base_addr;
	if (!rb || rb->magic != BL30MSG_MAGIC) {
		pr_err("ring buffer is NULL or MAGIC value is error\n");
		return NULL;
	}

	if (!rb->len || rb->len > BL30MSG_BUF_SIZE || rb->size > BL30MSG_BUF_SIZE) {
		pr_info("ring buffer size is error\n");
		return NULL;
	}

	amp_lock_obtain(&rb->lock, ARM_CPU);

	buf_len = rb->len < LOG_BUF_SIZE ? rb->len : LOG_BUF_SIZE;
	*buf_size = buf_len;
//	rb_head = rb->head;
	while (buf_len--) {
		//log_buf_aocpu[count++] = rb->data[rb_head++];
		//rb_head %= rb->size;
		log_buf_aocpu[count++] = rb->data[rb->head++];
		rb->head %= rb->size;
		rb->len--;
	}
	amp_lock_release(&rb->lock, ARM_CPU);

	log_buf_aocpu[count] = '\0';
	iounmap(bl30_buf_base_addr);
	return log_buf_aocpu;
}
EXPORT_SYMBOL(aocpu_log_dump);

static int aocpu_log_dump_pm_event(struct notifier_block *notifier,
			unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_POST_SUSPEND:/*exit suspend pm event.*/
		printk("******aocpu log start*******\n");
		log_external_print(AOCPU_LOG);
		printk("******aocpu log end*******\n");
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block aocpu_log_pm_notifier_block = {
	.notifier_call = aocpu_log_dump_pm_event,
};

static int __init uboot_log_init(void)
{
	struct proc_dir_entry *entry;
	int ret;

	if (log_paddr && log_size) {
		entry = proc_create("bl_log", 0444, NULL, &uboot_log_file_ops);
		if (!entry)
			pr_err("%s: failed to create proc entry\n", __func__);
	}
	pr_info("%s\n", __func__);

	ret = register_pm_notifier(&aocpu_log_pm_notifier_block);
	if (ret)
		pr_err("aocpu_log:register_pm_notifier Failed\n");

	pr_info("aocpu_log init ok\n");
	return 0;
}

static void __exit uboot_log_exit(void)
{
	remove_proc_entry("bl_log", NULL);
	if (log_vaddr)
		vunmap(log_vaddr);

	unregister_pm_notifier(&aocpu_log_pm_notifier_block);
}

module_init(uboot_log_init);
module_exit(uboot_log_exit);
/*
static int __init uboot_log_setup(char *str)
{
	char *tmp;
	int ret;

	tmp = strchr(str, ',');
	*tmp = 0;
	tmp++;

	ret = kstrtoul(str, 16, (unsigned long *)&log_paddr);
	if (ret)
		return 1;
	ret = kstrtoul(tmp, 10, &log_size);
	if (ret)
		return 1;

	memblock_reserve(log_paddr, log_size);
	return 1;
}
__setup("uboot_log=", uboot_log_setup);

*/
static int uboot_mem_device_init(struct reserved_mem *rmem, struct device *dev)
{
	pr_info("%s!\n", __func__);
	return 0;
}

static const struct reserved_mem_ops rmem_uboot_ops = {
	.device_init =  uboot_mem_device_init,
};

static int __init uboot_mem_setup(struct reserved_mem *rmem)
{
	rmem->ops = &rmem_uboot_ops;
	log_paddr = rmem->base;
	log_size  = rmem->size;
	pr_info("%s, base=%p , size=0x%lx\n",
			__func__, (void *)log_paddr, log_size);
	return 0;
}

RESERVEDMEM_OF_DECLARE(uboot_log, "bl_log", uboot_mem_setup);

MODULE_DESCRIPTION("export uboot log driver");


#include <linux/stacktrace.h>
#include <linux/export.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/irqflags.h>
#include <linux/sched.h>
#include "../kernel/sched/sched.h"
#include <linux/moduleparam.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

static unsigned int stack_margin = (1024 * 2);
static inline unsigned int stack_usage(void)
{
	register unsigned long sp asm ("sp");
	return  (unsigned int)(((sp + THREAD_SIZE) & ~(THREAD_SIZE - 1)) - sp);
}

void __attribute__((__no_instrument_function__))  __cyg_profile_func_exit(
	void *this_fn, void *call_site) {}
EXPORT_SYMBOL(__cyg_profile_func_exit);
void __attribute__((__no_instrument_function__))  __cyg_profile_func_enter(
	void *this_fn, void *call_site)
{
	static unsigned int fffflg;
	unsigned int stack_size;
	static unsigned long long t0;
	unsigned long long t1;
	if (fffflg)
		return;
	fffflg = 1;
	stack_size = stack_usage();
	if (stack_size >= THREAD_SIZE - stack_margin) {
		t1 = sched_clock();
		if (t1 - t0 > 1000000*500) {
			pr_err(" STACK__WARNING. %s,  stack:%d, margin:%d\n",
				current->comm, stack_size, stack_margin);
			dump_stack();
		}
		t0 = t1;
	}
	fffflg = 0;
}
EXPORT_SYMBOL(__cyg_profile_func_enter);


void test_stack(void)
{
#define BUF_STEP 256
	static unsigned int cnttt;
	int buf[BUF_STEP/sizeof(int)];
	int i = 0;

	cnttt++;
	pr_err(" A____A %d:  %d\n", cnttt, stack_usage());
	for (i = 0; i < BUF_STEP/sizeof(int); i++)
		buf[i] = 0x12345678;
	test_stack();
	mdelay(500);
}



static ssize_t stack_margin_write(struct file *file, const char __user *userbuf,
				   size_t count, loff_t *ppos)
{
	char buf[10];
	count = min_t(size_t, count, (sizeof(buf)-1));
	if (copy_from_user(buf, userbuf, count))
		return -EFAULT;
	buf[count] = 0;
	count = sscanf(buf, "%d", &stack_margin);
	pr_err("%s:%d\n", __func__, stack_margin);
	return count;
}
static ssize_t stack_margin_read(struct file *file, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t len;
	len = snprintf(buf, sizeof(buf), "%d\n", stack_margin);
	pr_err("%s:%d\n", __func__, stack_margin);
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}

static const struct file_operations stack_margin_ops = {
	.open		= simple_open,
	.read		= stack_margin_read,
	.write		= stack_margin_write,
};

static ssize_t stack_debug_write(struct file *file, const char __user *userbuf,
				   size_t count, loff_t *ppos)
{
	pr_info("%s:%d\n", __func__, (unsigned int)stack_margin);
	test_stack();
	return count;
}
static ssize_t stack_debug_read(struct file *file, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	pr_err("%s\n", __func__);
	return count;
}
static const struct file_operations stack_debug_ops = {
	.open		= simple_open,
	.read		= stack_debug_read,
	.write		= stack_debug_write,
};

static int __init debug_stack_init(void)
{
	struct dentry *dir = debugfs_create_dir("debug_stack", NULL);
	if (IS_ERR_OR_NULL(dir)) {
		pr_warn("failed to create debug_lockup\n");
		dir = NULL;
		return -1;
	}
	debugfs_create_file("stack_margin", S_IFREG | S_IRUGO,
			    dir, NULL, &stack_margin_ops);
	debugfs_create_file("stack_debug", S_IFREG | S_IRUGO,
			    dir, NULL, &stack_debug_ops);
	return 0;
}
late_initcall(debug_stack_init);

MODULE_DESCRIPTION("Amlogic debug stack module");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jianxin Pan <jianxin.pan@amlogic.com>");


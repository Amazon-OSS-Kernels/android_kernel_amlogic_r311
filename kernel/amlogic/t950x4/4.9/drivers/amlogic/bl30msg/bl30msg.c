/* SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 *
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>

#include "ring_buffer.h"

struct ring_buffer *bl30_msg_buffer;
struct proc_dir_entry *bl30msg_proc;
void __iomem *bl30msg_base;

static int bl30msg_proc_show(struct seq_file *m, void *v)
{
	char *info, *tmp;
	unsigned int count, start;

	count = bl30_msg_buffer->len;
	start = bl30_msg_buffer->head;
	info = kzalloc(bl30_msg_buffer->size, GFP_KERNEL);
	tmp = info;
	while (count--) {
		*(tmp++) = bl30_msg_buffer->data[start++];
		start %= bl30_msg_buffer->size;
	}

	seq_write(m, info, bl30_msg_buffer->len);
	kfree(info);

	return 0;
}

static int bl30msg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bl30msg_proc_show, NULL);
}

static const struct file_operations bl30msg_proc_fops = {
	.open = bl30msg_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static int bl30msg_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	pr_warn("[%s]begin\n", __func__);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_warn("failed to get memory resource.\n");
		return -ENXIO;
	}

	bl30msg_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(bl30msg_base))
		return PTR_ERR(bl30msg_base);

	bl30_msg_buffer = (struct ring_buffer *)bl30msg_base;
	bl30msg_proc = proc_create("bl30msg", 0644, NULL, &bl30msg_proc_fops);
	pr_warn("[%s]finished\n", __func__);
	return 0;
}

static int bl30msg_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	devm_iounmap(dev, bl30msg_base);
	proc_remove(bl30msg_proc);
	return 0;
}

static const struct of_device_id bl30msg_of_match[] = {
	{ .compatible = "amlogic, meson_bl30msg" },
	{}
};

static struct platform_driver bl30msg_driver = {
	.probe = bl30msg_probe,
	.remove = bl30msg_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "meson_bl30msg",
		.of_match_table = bl30msg_of_match,
	}
};

static int __init bl30msg_init(void)
{
  	pr_warn("[%s]module init begin\n", __func__);
	return platform_driver_register(&bl30msg_driver);
}
core_initcall(bl30msg_init);

static void __exit bl30msg_exit(void)
{
	platform_driver_unregister(&bl30msg_driver);
}
module_exit(bl30msg_exit);

MODULE_AUTHOR("xingxing.wang@amlogic.com");
MODULE_DESCRIPTION("meson bl30msg driver");
MODULE_LICENSE("GPL");

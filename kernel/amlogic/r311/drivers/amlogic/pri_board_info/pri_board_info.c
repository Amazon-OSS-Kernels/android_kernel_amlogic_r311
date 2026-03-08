/*
 * board info driver for Jane
 *
 * Copyright (C) 2017 Technologies inc.
 * xuhua.zhang <xuhua.zhang@amlogic.com>
 * Copyright (C) 2017 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#define OWNER_NAME "pri_board_info"

struct class board_info_class;
struct gpio_desc *hw_id_1;
struct gpio_desc *hw_id_2;
struct gpio_desc *hw_id_3;
struct gpio_desc *hw_id_4;
struct gpio_desc *ssw_id;
char hwid_val[10] = "";

static ssize_t hw_id_show(struct class *cls,
				struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", hwid_val);
}

static ssize_t hw_id_store(struct class *cls,
			   struct class_attribute *attr,
			   const char *buf, size_t size)
{
	return size;
}

struct class_attribute board_info_attrs[] = {
	__ATTR_RW(hw_id),
	__ATTR_NULL,
};

static int board_info_probe(struct platform_device *pdev)
{
	int i, ret;
	int val[5];
	char buf[10];
	struct device_node *node = pdev->dev.of_node;

	hw_id_1 = of_get_named_gpiod_flags(node, "hw_id_1", 0, NULL);
	hw_id_2 = of_get_named_gpiod_flags(node, "hw_id_2", 0, NULL);
	hw_id_3 = of_get_named_gpiod_flags(node, "hw_id_3", 0, NULL);
	hw_id_4 = of_get_named_gpiod_flags(node, "hw_id_4", 0, NULL);
	ssw_id = of_get_named_gpiod_flags(node, "ssw_id", 0, NULL);

	ret  = gpio_request(desc_to_gpio(hw_id_1), OWNER_NAME);
	ret |= gpio_request(desc_to_gpio(hw_id_2), OWNER_NAME);
	ret |= gpio_request(desc_to_gpio(hw_id_3), OWNER_NAME);
	ret |= gpio_request(desc_to_gpio(hw_id_4), OWNER_NAME);
	ret |= gpio_request(desc_to_gpio(ssw_id), OWNER_NAME);
	if (ret) {
		pr_info("pri_board_info gpio request error\n");
		return ret;
	}

	ret  = gpiod_direction_input(hw_id_1);
	ret |= gpiod_direction_input(hw_id_2);
	ret |= gpiod_direction_input(hw_id_3);
	ret |= gpiod_direction_input(hw_id_4);
	ret |= gpiod_direction_input(ssw_id);
	if (ret) {
		pr_info("pri_board_info gpio init error\n");
		return ret;
	}

	val[0] = gpiod_get_value(hw_id_4);
	val[1] = gpiod_get_value(hw_id_3);
	val[2] = gpiod_get_value(hw_id_2);
	val[3] = gpiod_get_value(hw_id_1);
	val[4] = gpiod_get_value(ssw_id);

	for (i = 0; i < 4; ++i) {
		sprintf(buf, "%d", val[i]);
		strcat(hwid_val, buf);
	}

	/* init class */
	board_info_class.name = OWNER_NAME;
	board_info_class.owner = THIS_MODULE;
	board_info_class.class_attrs = board_info_attrs;
	ret = class_register(&board_info_class);
	if (ret) {
		pr_err("failed to create board info class.\n");
		return ret;
	}

	pr_info("SSW ID is %d\n", val[4]);
	pr_info("HW ID is %s\n", hwid_val);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id board_info_dt_match[] = {
	{
		.compatible = "amlogic, board_info",
	},
	{},
};
#endif

static struct platform_driver board_info_driver = {
	.probe   = board_info_probe,
	.driver  = {
		.name  = "board_info",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = board_info_dt_match,
#endif
	},
};

static int __init board_info_init(void)
{
	return platform_driver_register(&board_info_driver);
}

static void __exit board_info_exit(void)
{
	platform_driver_unregister(&board_info_driver);
}

module_init(board_info_init);
module_exit(board_info_exit);

MODULE_DESCRIPTION("Meson Board Info Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic Platform team");

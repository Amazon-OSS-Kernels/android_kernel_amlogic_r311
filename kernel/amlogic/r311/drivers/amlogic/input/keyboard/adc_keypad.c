/*
 * drivers/amlogic/input/adc_keypad/adc_keypad.c
 *
 * ADC Keypad Driver
 *
 * Copyright (C) 2017 Amlogic, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/amlogic/saradc.h>
#include <linux/amlogic/input/adc_keypad.h>
#include <linux/amlogic/scpi_protocol.h>
#include <linux/amlogic/pm.h>
#include <linux/amlogic/scpi_protocol.h>
#include <linux/silent_ota_backlight.h>
#ifdef CONFIG_5_WAY_KEYS
#include <linux/amlogic/vout/aml_bl.h>
#endif
#define LONG_PRESS_DET_THRESHOLD  100 /* 100 * 20ms = 2s */
#define POLL_PERIOD_WHEN_KEY_DOWN 20 /* unit msec */
#define POLL_PERIOD_WHEN_KEY_UP   50
#define KEY_JITTER_COUNT  1  /*  1 * POLL_PERIOD_WHEN_KEY_DOWN msec  */
#define TMP_BUF_MAX 128

static char adc_key_mode_name[MAX_NAME_LEN] = "abcdef";
static char kernelkey_en_name[MAX_NAME_LEN] = "abcdef";
static bool keypad_enable_flag = 1;
static unsigned int str_count;
static unsigned int power_button; /* 1: power button, 0: key down button */

static int __init power_button_setup(char *str)
{
	int ret = 0;

	if (str != NULL)
		ret = kstrtouint(str, 10, &power_button);

	return ret;
}
__setup("silent_ota=", power_button_setup);

#ifdef CONFIG_5_WAY_KEYS
static int key_check_for_power_button(unsigned int code, int value)
{
        int backlight_on_req = 0;

        /* normal boot, code is KEY_POWER, recover is KEY_ENTER */
        if (!boot_complete && !value &&
                        ((code == KEY_ENTER && power_button) ||
                         code == KEY_POWER))
                backlight_on_req = OTA_TOGGLE_BACKLIGHT;

        if (backlight_on_req) {
                toggle_backlight(backlight_on_req);
        }

        return 0;
}
#else
static int key_check_for_power_button(unsigned int code, int value)
{
	int backlight_on_req = 0;

	/* normal boot, code is KEY_POWER, recover is KEY_DOWN */
	if (!boot_complete && !value &&
			((code == KEY_DOWN && power_button) ||
			 code == KEY_POWER))
		backlight_on_req = OTA_TOGGLE_BACKLIGHT;

	if (backlight_on_req) {
		toggle_backlight(backlight_on_req);
	}

	return 0;
}
#endif
static void kp_report_key(struct kp *kp, unsigned int code, int value)
{
	if (code == kp->keymap.code) {
		if (value)
			mod_timer(&kp->lp_timer,
				jiffies +
				msecs_to_jiffies(POLL_PERIOD_WHEN_KEY_DOWN));
		else
			set_bit(KP_KEY_MAP_STATE, &kp->keymap.flags);
	} else {
		dev_info(&kp->input->dev, "key %d %s\n",
			code, value ? "down" : "up");
		key_check_for_power_button(code, value);
		input_report_key(kp->input, code, value);
		input_sync(kp->input);
	}
}

static int kp_search_key(struct kp *kp)
{
	struct adc_key *key;
	int value, i;

	spin_lock(&kp->kp_lock);
	for (i = 0; i < kp->chan_num; i++) {
		value = get_adc_sample(0, kp->chan[i]);
		if (value < 0)
			continue;
		list_for_each_entry(key, &kp->adckey_head, list) {
			if ((key->chan == kp->chan[i])
			&& (value >= key->value - key->tolerance)
			&& (value <= key->value + key->tolerance)) {
				spin_unlock(&kp->kp_lock);
				return key->code;
			}
		}
	}
	spin_unlock(&kp->kp_lock);
	return 0;
}

static void kp_work(struct kp *kp)
{
	int code = kp_search_key(kp);
	if (code)
		kp->poll_period = POLL_PERIOD_WHEN_KEY_DOWN;
	if ((!code) && (!kp->report_code)) {
		if (kp->poll_period < POLL_PERIOD_WHEN_KEY_UP)
			kp->poll_period++;
		return;
	} else if (code != kp->code) {
		kp->code = code;
		kp->count = 0;
	} else if (kp->count < KEY_JITTER_COUNT) {
		kp->count++;
		} else {
			if ((kp->report_code != code) && keypad_enable_flag) {
				if (!code) { /* key up */
					kp_report_key(kp, kp->report_code, 0);
				} else if (!kp->report_code) { /* key down */
					kp_report_key(kp, code, 1);
				} else {
					kp_report_key(kp, kp->report_code, 0);
					kp_report_key(kp, code, 1);
				}
			kp->report_code = code;
		}
	}
}

static void update_work_func(struct work_struct *work)
{
	struct kp *kp = container_of(work, struct kp, work_update);
	kp_work(kp);
}

static void kp_timer_sr(unsigned long data)
{
	struct kp *kp = (struct kp *)data;
	schedule_work(&(kp->work_update));
	mod_timer(&kp->timer, jiffies + msecs_to_jiffies(kp->poll_period));
}

static void kp_lp_timer_sr(unsigned long data)
{
	struct kp *kp = (struct kp *)data;

	kp->keymap.timestamp++;

	if (kp->keymap.timestamp == LONG_PRESS_DET_THRESHOLD) {
		dev_info(&kp->input->dev, "key %d down\n", kp->keymap.lcode);
		input_report_key(kp->input, kp->keymap.lcode, 1);
		input_sync(kp->input);
	}

	if (test_and_clear_bit(KP_KEY_MAP_STATE, &kp->keymap.flags)) {
		if (kp->keymap.timestamp < LONG_PRESS_DET_THRESHOLD) {
#ifdef CONFIG_5_WAY_KEYS
			if((get_backlight_status() == 0)&&(kp->keymap.scode == KEY_ENTER))
			{
				//if black screen, enter key acts like power key
				dev_info(&kp->input->dev,
				"black screen key %d down\n", KEY_POWER);
				input_report_key(kp->input, KEY_POWER, 1);
				input_sync(kp->input);
				dev_info(&kp->input->dev,
					"black screen key %d up\n", KEY_POWER);
				input_report_key(kp->input, KEY_POWER, 0);
				key_check_for_power_button(KEY_POWER, 0);
				input_sync(kp->input);
			}
			else{
#endif
			dev_info(&kp->input->dev,
				"key %d down\n", kp->keymap.scode);
			input_report_key(kp->input, kp->keymap.scode, 1);
			input_sync(kp->input);
			dev_info(&kp->input->dev,
				"key %d up\n", kp->keymap.scode);
			input_report_key(kp->input, kp->keymap.scode, 0);
			key_check_for_power_button(kp->keymap.scode, 0);
			input_sync(kp->input);
#ifdef CONFIG_5_WAY_KEYS
			}
#endif
		} else {
			dev_info(&kp->input->dev,
				"key %d up\n", kp->keymap.lcode);
			input_report_key(kp->input, kp->keymap.lcode, 0);
			input_sync(kp->input);
		}
		kp->keymap.timestamp = 0;
	} else {
		mod_timer(&kp->lp_timer,
			jiffies + msecs_to_jiffies(POLL_PERIOD_WHEN_KEY_DOWN));
	}
}

#if 0/*CONFIG_HAS_EARLYSUSPEND*/
static void kp_early_suspend(struct early_suspend *h)
{
	struct kp *kp = container_of(h, struct kp, early_suspend);
	del_timer_sync(&kp->timer);
	cancel_work_sync(&kp->work_update);
}

static void kp_late_resume(struct early_suspend *h)
{
	struct kp *kp = container_of(h, struct kp, early_suspend);
	 mod_timer(&kp->timer, jiffies+msecs_to_jiffies(kp->poll_period));
}
#endif

static void send_data_to_bl301(void)
{
	u32 val;
	if (!strcmp(adc_key_mode_name, "POWER_WAKEUP_POWER")) {
		val = 0;  /*only power key resume*/
		scpi_send_usr_data(SCPI_CL_POWER, &val, sizeof(val));
	} else if (!strcmp(adc_key_mode_name, "POWER_WAKEUP_ANY")) {
		val = 1; /*any key resume*/
		scpi_send_usr_data(SCPI_CL_POWER, &val, sizeof(val));
	} else if (!strcmp(adc_key_mode_name, "POWER_WAKEUP_NONE")) {
		val = 2; /*no key can resume*/
		scpi_send_usr_data(SCPI_CL_POWER, &val, sizeof(val));
	}

}

static void kernel_keypad_enable_mode_enable(void)
{

	if (!strcmp(kernelkey_en_name, "KEYPAD_UNLOCK")) {
		keypad_enable_flag = 1;  /*unlock, normal mode*/
	} else if (!strcmp(kernelkey_en_name, "KEYPAD_LOCK")) {
		keypad_enable_flag = 0;  /*lock, press key will be not usefull*/
	} else {
		keypad_enable_flag = 1;
	}
}

/*kp_valid_chan_update() - used to update valid adc channel
 *
 *@kp: to save number of channel in
 *
 */
static void kp_valid_chan_update(struct kp *kp)
{
	unsigned char incr;
	struct adc_key *key;

	spin_lock(&kp->kp_lock);
	kp->chan_num = 0; /*recalculate*/
	list_for_each_entry(key, &kp->adckey_head, list) {
		if (0 == kp->chan_num) {
			kp->chan[kp->chan_num++] = key->chan;
		} else {
			for (incr = 0; incr < kp->chan_num; incr++) {
				if (key->chan == kp->chan[incr])
					break;
				else
					if (incr == (kp->chan_num - 1))
						kp->chan[kp->chan_num++]
							= key->chan;
			}
		}
	}
	spin_unlock(&kp->kp_lock);
}
static int kp_get_devtree_pdata(struct platform_device *pdev, struct kp *kp)
{
	int ret;
	int state = 0;
	unsigned char cnt;
	const char *uname;
	unsigned int key_num;
	struct adc_key *key;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "pdev->dev.of_node == NULL!\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "key_num", &key_num);
	if (ret) {
		dev_err(&pdev->dev, "faild to get key_num!\n");
		return -EINVAL;
	}
	for (cnt = 0; cnt < key_num; cnt++) {
		key = kzalloc(sizeof(struct adc_key), GFP_KERNEL);
		if (!key) {
			dev_err(&pdev->dev, "alloc mem failed!\n");
			return -ENOMEM;
		}

		ret = of_property_read_string_index(pdev->dev.of_node,
			 "key_name", cnt, &uname);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key name index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}
		strncpy(key->name, uname, MAX_NAME_LEN);

		ret = of_property_read_u32_index(pdev->dev.of_node,
			"key_code", cnt, &key->code);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key code index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}

		ret = of_property_read_u32_index(pdev->dev.of_node,
			"key_chan", cnt, &key->chan);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key chan index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}

		ret = of_property_read_u32_index(pdev->dev.of_node,
			"key_val", cnt, &key->value);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key value index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}

		ret = of_property_read_u32_index(pdev->dev.of_node,
			"key_tolerance", cnt, &key->tolerance);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key tolerance index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}
		spin_lock(&kp->kp_lock);
		set_bit(key->code, kp->input->keybit); /*set event code*/
		list_add_tail(&key->list, &kp->adckey_head);
		spin_unlock(&kp->kp_lock);
	}
	kp_valid_chan_update(kp);
	return 0;
err:
	kfree(key);
	return state;

}

static void kp_list_free(struct kp *kp)
{
	struct adc_key *key;
	struct adc_key *key_tmp;

	spin_lock(&kp->kp_lock);
	list_for_each_entry_safe(key, key_tmp, &kp->adckey_head, list) {
		list_del(&key->list);
		kfree(key);
	}
	spin_unlock(&kp->kp_lock);
}

static ssize_t table_show(struct class *cls, struct class_attribute *attr,
			char *buf)
{
	struct kp *kp = container_of(cls, struct kp, kp_class);
	struct adc_key *key;
	unsigned char key_num = 1;
	int len = 0;

	spin_lock(&kp->kp_lock);
	list_for_each_entry(key, &kp->adckey_head, list) {
		len += sprintf(buf+len,
			"[%d]: name=%-21s code=%-5d channel=%-3d value=%-5d tolerance=%-5d\n",
			key_num,
			key->name,
			key->code,
			key->chan,
			key->value,
			key->tolerance);
		key_num++;
	}
	spin_unlock(&kp->kp_lock);

	return len;
}

static ssize_t table_store(struct class *cls, struct class_attribute *attr,
			 const char *buf, size_t count)
{
	struct kp *kp = container_of(cls, struct kp, kp_class);
	struct device *dev = kp->input->dev.parent;
	struct adc_key *dkey;
	struct adc_key *key;
	struct adc_key *key_tmp;
	char nbuf[TMP_BUF_MAX];
	char *pbuf = nbuf;
	unsigned char colon_num = 0;
	int nsize = 0;
	int state = 0;
	char *pval;

	/*count inclued '\0'*/
	if (count > TMP_BUF_MAX) {
		dev_err(dev, "write data is too long[max:%d]: %zu\n",
			TMP_BUF_MAX, count);
		return -EINVAL;
	}

	/*trim all invisible characters include '\0', tab, space etc*/
	while (*buf) {
		if (*buf > ' ')
			nbuf[nsize++] = *buf;
		if (*buf == ':')
			colon_num++;
		buf++;
	}
	nbuf[nsize] = '\0';

	/*write "null" or "NULL" to clean up all key table*/
	if (strcasecmp("null", nbuf) == 0) {
		kp_list_free(kp);
		return count;
	}

	/*to judge write data format whether valid or not*/
	if (colon_num != 4) {
		dev_err(dev, "write data invalid: %s\n", nbuf);
		dev_err(dev, "=> [name]:[code]:[channel]:[value]:[tolerance]\n");
		return -EINVAL;
	}

	dkey = kzalloc(sizeof(struct adc_key), GFP_KERNEL);
	if (!dkey) {
		dev_err(dev, "alloc mem failed!\n");
		return -ENOMEM;
	}

	/*save the key data in order*/
	pval = strsep(&pbuf, ":"); /*name*/
	if (pval)
		strncpy(dkey->name, pval, MAX_NAME_LEN);

	pval = strsep(&pbuf, ":"); /*code*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->code) < 0) {
			state = -EINVAL;
			goto err;
		}
	pval = strsep(&pbuf, ":"); /*channel*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->chan) < 0) {
			state = -EINVAL;
			goto err;
		}
	pval = strsep(&pbuf, ":"); /*value*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->value) < 0) {
			state = -EINVAL;
			goto err;
		}
	pval = strsep(&pbuf, ":"); /*tolerance*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->tolerance) < 0) {
			state = -EINVAL;
			goto err;
		}

	/*check channel data whether valid or not*/
	if (dkey->chan >= SARADC_CHAN_NUM) {
		dev_err(dev, "invalid channel[%d-%d]: %d\n", 0,
			SARADC_CHAN_NUM-1, dkey->chan);
		state = -EINVAL;
		goto err;
	}

	/*check sample data whether valid or not*/
	if (dkey->value > SAM_MAX) {
		dev_err(dev, "invalid sample value[%d-%d]: %d\n",
			SAM_MIN, SAM_MAX, dkey->value);
		state = -EINVAL;
		goto err;
	}

	/*check tolerance data whether valid or not*/
	if (dkey->tolerance > TOL_MAX) {
		dev_err(dev, "invalid tolerance[%d-%d]: %d\n",
			TOL_MIN, TOL_MAX, dkey->tolerance);
		state = -EINVAL;
		goto err;
	}

	spin_lock(&kp->kp_lock);
	list_for_each_entry_safe(key, key_tmp, &kp->adckey_head, list) {
		if ((key->code == dkey->code) ||
			((key->chan == dkey->chan) &&
			(key->value == dkey->value))) {
			dev_info(dev, "del older key => %s:%d:%d:%d:%d\n",
				key->name, key->code, key->chan,
				key->value, key->tolerance);
			clear_bit(key->code,  kp->input->keybit);
			list_del(&key->list);
			kfree(key);
		}
	}
	set_bit(dkey->code,  kp->input->keybit);
	list_add_tail(&dkey->list, &kp->adckey_head);
	dev_info(dev, "add newer key => %s:%d:%d:%d:%d\n", dkey->name,
		dkey->code, dkey->chan, dkey->value, dkey->tolerance);
	spin_unlock(&kp->kp_lock);

	kp_valid_chan_update(kp);

	return count;
err:
	kfree(dkey);
	return state;
}

static ssize_t keymap_store(struct class *cls, struct class_attribute *attr,
			const char *buf, size_t count)
{
	struct kp *kp = container_of(cls, struct kp, kp_class);
	struct device *dev = kp->input->dev.parent;
	char nbuf[TMP_BUF_MAX];
	char *pbuf = nbuf;
	u8 colon_num = 0;
	int nsize = 0;
	char *pval;

	/*count inclued '\0'*/
	if (count > TMP_BUF_MAX) {
		dev_err(dev, "write data is too long[max:%d]: %zu\n",
			TMP_BUF_MAX, count);
		return -EINVAL;
	}

	/*trim all invisible characters include '\0', tab, space etc*/
	while (*buf) {
		if (*buf > ' ')
			nbuf[nsize++] = *buf;
		if (*buf == ':')
			colon_num++;
		buf++;
	}

	/*to judge write data format whether valid or not*/
	if (colon_num != 2) {
		dev_err(dev, "write data invalid: %s\n", nbuf);
		dev_err(dev, "=> [code]:[sp_code]:[lp_code]\n");
		return -EINVAL;
	}

	nbuf[nsize] = '\0';

	if (kp->keymap.scode)
		clear_bit(kp->keymap.scode, kp->input->keybit);

	if (kp->keymap.lcode)
		clear_bit(kp->keymap.lcode, kp->input->keybit);

	pval = strsep(&pbuf, ":"); /*code*/
	if (pval && kstrtoint(pval, 0, &kp->keymap.code) < 0)
		return -EINVAL;

	pval = strsep(&pbuf, ":"); /*short-press code*/
	if (pval && kstrtoint(pval, 0, &kp->keymap.scode) < 0)
		return -EINVAL;

	pval = strsep(&pbuf, ":"); /*long-press code*/
	if (pval && kstrtoint(pval, 0, &kp->keymap.lcode) < 0)
		return -EINVAL;

	set_bit(kp->keymap.scode, kp->input->keybit);
	set_bit(kp->keymap.lcode, kp->input->keybit);

	dev_info(dev, "[%d] map to [short-press: %d] and [long-press: %d]\n",
		kp->keymap.code, kp->keymap.scode, kp->keymap.lcode);

	return count;
}

static ssize_t str_count_show(struct class *cls, struct class_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%u\n", str_count);
}

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct class_attribute kp_attrs[] = {
	__ATTR_RW(table),
	__ATTR_WO(keymap),
	__ATTR_RO(str_count),
	__ATTR_NULL
};

static int kp_probe(struct platform_device *pdev)
{
	struct kp *kp;
	int ret = 0;

	send_data_to_bl301();
	kernel_keypad_enable_mode_enable();

	kp = devm_kzalloc(&pdev->dev, sizeof(struct kp), GFP_KERNEL);
	if (!kp) {
		dev_err(&pdev->dev, "alloc kp memory failed!\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, kp);
	spin_lock_init(&kp->kp_lock);
	INIT_LIST_HEAD(&kp->adckey_head);
	kp->report_code = 0;
	kp->code = 0;
	kp->poll_period = POLL_PERIOD_WHEN_KEY_UP;
	kp->count = 0;
	setup_timer(&kp->timer, kp_timer_sr, (unsigned long)kp);
	setup_timer(&kp->lp_timer, kp_lp_timer_sr, (unsigned long)kp);

	/*alloc input device*/
	kp->input = input_allocate_device();
	if (!kp->input) {
		dev_err(&pdev->dev, "alloc input device failed!\n");
		return -ENOMEM;
	}

	/* init input device */
	set_bit(EV_KEY, kp->input->evbit);
	set_bit(EV_REP, kp->input->evbit);
	kp->input->name = "adc_keypad";
	kp->input->phys = "adc_keypad/input0";
	kp->input->dev.parent = &pdev->dev;

	kp->input->id.bustype = BUS_ISA;
	kp->input->id.vendor = 0x0001;
	kp->input->id.product = 0x0001;
	kp->input->id.version = 0x0100;

	kp->input->rep[REP_DELAY] = 0xffffffff;
	kp->input->rep[REP_PERIOD] = 0xffffffff;

	kp->input->keycodesize = sizeof(unsigned short);
	kp->input->keycodemax = 0x1ff;

	/*register input device*/
	ret = input_register_device(kp->input);
	if (ret) {
		dev_err(&pdev->dev,
			 "unable to register keypad input device.\n");
		goto input_retister_err;
	}

	ret = kp_get_devtree_pdata(pdev, kp);
	if (ret)
		goto devtree_err;

	/*init class*/
	kp->kp_class.name = DRIVE_NAME;
	kp->kp_class.owner = THIS_MODULE;
	kp->kp_class.class_attrs = kp_attrs;
	ret = class_register(&kp->kp_class);
	if (ret) {
		dev_err(&pdev->dev, "fail to create adc keypad class.\n");
		goto devtree_err;
	}

	INIT_WORK(&(kp->work_update), update_work_func);

	#if 0/*CONFIG_HAS_EARLYSUSPEND*/
	kp->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	kp->early_suspend.suspend = kp_early_suspend;
	kp->early_suspend.resume = kp_late_resume;
	register_early_suspend(&kp->early_suspend);
	#endif

	/*enable timer*/
	mod_timer(&kp->timer, jiffies+msecs_to_jiffies(100));

	return 0;

devtree_err:
	kp_list_free(kp);
	input_unregister_device(kp->input);
input_retister_err:
	input_free_device(kp->input);

	return ret;
}

static int kp_remove(struct platform_device *pdev)
{
	struct kp *kp = platform_get_drvdata(pdev);

	class_unregister(&kp->kp_class);
	#if 0/*CONFIG_HAS_EARLYSUSPEND*/
	unregister_early_suspend(&kp->early_suspend);
	#endif
	del_timer_sync(&kp->timer);
	del_timer_sync(&kp->lp_timer);
	cancel_work_sync(&kp->work_update);
	input_unregister_device(kp->input);
	input_free_device(kp->input);
	kp_list_free(kp);
	return 0;
}

static int kp_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int kp_resume(struct platform_device *pdev)
{
	struct kp *kp = platform_get_drvdata(pdev);

	str_count++;

	if (get_resume_method() == POWER_KEY_WAKEUP) {
		dev_info(&pdev->dev, "adc keypad wakeup\n");
		input_report_key(kp->input ,  KEY_POWER ,  1);
		input_sync(kp->input);
		input_report_key(kp->input ,  KEY_POWER ,  0);
		input_sync(kp->input);
		if (scpi_clr_wakeup_reason())
			pr_debug("clr wakeup reason fail.\n");
	}
	return 0;
}

static const struct of_device_id key_dt_match[] = {
	{.compatible = "amlogic, adc_keypad",},
	{},
};

static struct platform_driver kp_driver = {
	.probe      = kp_probe,
	.remove     = kp_remove,
	.suspend    = kp_suspend,
	.resume     = kp_resume,
	.driver     = {
		.name   = DRIVE_NAME,
		.of_match_table = key_dt_match,
	},
};

static int __init kp_init(void)
{
	return platform_driver_register(&kp_driver);
}

static void __exit kp_exit(void)
{
	platform_driver_unregister(&kp_driver);
}

static int __init adc_key_mode_para_setup(char *s)
{
	if (NULL != s)
		sprintf(adc_key_mode_name, "%s", s);

	return 0;
}
__setup("adckeyswitch=", adc_key_mode_para_setup);

static int __init kernel_keypad_enable_setup(char *s)
{
	if (NULL != s)
		sprintf(kernelkey_en_name, "%s", s);

	return 0;
}
__setup("kernelkey_enable=", kernel_keypad_enable_setup);

module_init(kp_init);
module_exit(kp_exit);
MODULE_AUTHOR("Amlogic");
MODULE_DESCRIPTION("ADC Keypad Driver");
MODULE_LICENSE("GPL V2");

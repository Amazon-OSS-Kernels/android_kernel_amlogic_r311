/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <asm/arch/cpu.h>
#include <config.h>
#include <common.h>
#include <errno.h>
#include <g_dnl.h>
#include <malloc.h>
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
#include <fb_mmc.h>
#include <fb_storage.h>
#endif
#if defined(CONFIG_IDME)
#include <idme.h>
#endif
#if defined(UFBL_FEATURE_UNLOCK)
#include <amzn_unlock.h>
#endif
#include <linux/ctype.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/compiler.h>
#include <partition_table.h>
#include <version.h>

#if defined(UFBL_FEATURE_UNLOCK)
#include "amzn_unlock.h"
#else
#error "UFBL_FEATURE_UNLOCK is required"
#endif

#if defined(UFBL_FEATURE_SECURE_BOOT)
#include "amzn_secure_boot.h"
#else
#error "UFBL_FEATURE_SECURE_BOOT is required"
#endif

#if defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
#include "amzn_fastboot_lockdown.h"
extern int is_locked_production_device();
#else
#error "UFBL_FEATURE_FASTBOOT_LOCKDOWN is required"
#endif

DECLARE_GLOBAL_DATA_PTR;

#define FASTBOOT_VERSION		"0.4"

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

#define DEVICE_PRODUCT	CONFIG_DEVICE_PRODUCT
#define DEVICE_SERIAL	"1234567890"

/* The 64 defined bytes plus \0 */
#define RESPONSE_LEN	(64 + 1)

#define EP_BUFFER_SIZE	4096

unsigned short fastboot_running = 0;

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}

static struct f_fastboot *fastboot_func;
static unsigned int download_size;
static unsigned int download_bytes;

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval          = 0x00,
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_in = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval          = 0x00,
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval		= 0x00,
};

static struct usb_interface_descriptor interface_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass	= FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= FASTBOOT_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *fb_runtime_descs[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&hs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

/*
 * static strings, in UTF-8
 */
static const char fastboot_name[] = "Android Fastboot";

static struct usb_string fastboot_string_defs[] = {
	[0].s = fastboot_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_fastboot = {
	.language	= 0x0409,	/* en-us */
	.strings	= fastboot_string_defs,
};

static struct usb_gadget_strings *fastboot_strings[] = {
	&stringtab_fastboot,
	NULL,
};

#if defined(CONFIG_BLANCHE)
extern bool secure_boot_enabled(void);
#else
static bool secure_boot_enabled(void)
{
	return false;
}
#endif

#define DRAM_UBOOT_RESERVE		0x01000000
unsigned int ddr_size_usable(unsigned int addr_start)
{
	unsigned int ddr_size=0;
	unsigned int free_size = 0;
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++)
		ddr_size += gd->bd->bi_dram[i].size;

	free_size = (ddr_size - DRAM_UBOOT_RESERVE - addr_start - CONFIG_SYS_MALLOC_LEN - CONFIG_SYS_MEM_TOP_HIDE);
#if defined CONFIG_FASTBOOT_MAX_DOWN_SIZE
	if (free_size > CONFIG_FASTBOOT_MAX_DOWN_SIZE)
		free_size = CONFIG_FASTBOOT_MAX_DOWN_SIZE;
#endif
	return free_size;
}

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;

	if (!status)
		return;
	printf("status: %d ep '%s' trans: %d\n", status, ep->name, req->actual);
}

static int fastboot_bind(struct usb_configuration *c, struct usb_function *f)
{
	int id;
	struct usb_gadget *gadget = c->cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	/* DYNAMIC interface numbers assignments */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	fastboot_string_defs[0].id = id;
	interface_desc.iInterface = id;

	f_fb->in_ep = usb_ep_autoconfig(gadget, &hs_ep_in);
	if (!f_fb->in_ep)
		return -ENODEV;
	f_fb->in_ep->driver_data = c->cdev;

	f_fb->out_ep = usb_ep_autoconfig(gadget, &hs_ep_out);
	if (!f_fb->out_ep)
		return -ENODEV;
	f_fb->out_ep->driver_data = c->cdev;

	hs_ep_out.bEndpointAddress = hs_ep_out.bEndpointAddress;

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	memset(fastboot_func, 0, sizeof(*fastboot_func));
}

static void fastboot_disable(struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_disable(f_fb->out_ep);
	usb_ep_disable(f_fb->in_ep);

	if (f_fb->out_req) {
		free(f_fb->out_req->buf);
		usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
		f_fb->out_req = NULL;
	}
	if (f_fb->in_req) {
		free(f_fb->in_req->buf);
		usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
		f_fb->in_req = NULL;
	}
}

static struct usb_request *fastboot_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;

	req->length = EP_BUFFER_SIZE;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
	return req;
}

static int fastboot_set_alt(struct usb_function *f,
			    unsigned interface, unsigned alt)
{
	int ret;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	/* make sure we don't enable the ep twice */
	ret = usb_ep_enable(f_fb->out_ep, &hs_ep_out);
	if (ret) {
		puts("failed to enable out ep\n");
		return ret;
	}

	f_fb->out_req = fastboot_start_ep(f_fb->out_ep);
	if (!f_fb->out_req) {
		puts("failed to alloc out req\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->out_req->complete = rx_handler_command;

	ret = usb_ep_enable(f_fb->in_ep, &hs_ep_in);
	if (ret) {
		puts("failed to enable in ep\n");
		goto err;
	}

	f_fb->in_req = fastboot_start_ep(f_fb->in_ep);
	if (!f_fb->in_req) {
		puts("failed alloc req in\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->in_req->complete = fastboot_complete;

	ret = usb_ep_queue(f_fb->out_ep, f_fb->out_req, 0);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable(f);
	return ret;
}

static int  fastboot_setup(struct usb_function *f,
	const struct usb_ctrlrequest *ctrl)
{
	int value = -EOPNOTSUPP;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	/* composite driver infrastructure handles everything; interface
	 * activation uses set_alt().
	 */
	if (((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT)
		&& (ctrl->bRequest == USB_REQ_CLEAR_FEATURE)
		&& (ctrl->wValue== USB_ENDPOINT_HALT)) {
		switch (ctrl->wIndex & 0xfe) {
		case USB_DIR_OUT:
			value = ctrl->wLength;
			usb_ep_clear_halt(f_fb->out_ep);
			break;

		case USB_DIR_IN:
			value = ctrl->wLength;
			usb_ep_clear_halt(f_fb->in_ep);
			break;
		default:
			printf("unknown usb_ctrlrequest\n");
			break;
		}
	}

	return value;
}

static int fastboot_add(struct usb_configuration *c)
{
	struct f_fastboot *f_fb = fastboot_func;
	int status;

	if (!f_fb) {
		f_fb = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*f_fb));
		if (!f_fb)
			return -ENOMEM;

		fastboot_func = f_fb;
		memset(f_fb, 0, sizeof(*f_fb));
	}

	f_fb->usb_function.name = "f_fastboot";
	f_fb->usb_function.hs_descriptors = fb_runtime_descs;
	f_fb->usb_function.bind = fastboot_bind;
	f_fb->usb_function.unbind = fastboot_unbind;
	f_fb->usb_function.set_alt = fastboot_set_alt;
	f_fb->usb_function.disable = fastboot_disable;
	f_fb->usb_function.strings = fastboot_strings;
	f_fb->usb_function.setup = fastboot_setup;

	status = usb_add_function(c, &f_fb->usb_function);
	if (status) {
		free(f_fb);
		fastboot_func = NULL;
	}

	return status;
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot, fastboot_add);

static int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req;
	int ret;

	if (fastboot_func)
		in_req = fastboot_func->in_req;
	else
		return 0;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;
	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

static int fastboot_tx_write_str(const char *buffer)
{
	return fastboot_tx_write(buffer, strlen(buffer));
}

int fastboot_info(const char *info)
{
	return fastboot_tx_write(info, strlen(info));
}

int fastboot_okay(const char *info)
{
	char response[64];
	memset(response, 0x0, sizeof(response));

	strncpy(response, "OKAY", strlen("OKAY"));
	strncat(response, info, strlen(info));
	return fastboot_tx_write(response, strlen(response));
}

int fastboot_fail(const char *reason)
{
	char response[64];
	memset(response, 0x0, sizeof(response));

	strncpy(response, "FAIL", strlen("FAIL"));
	strncat(response, reason, strlen(reason));
	return fastboot_tx_write(response, strlen(response));
}

/* flag bits related to adb on ABC. */
#define FOS_FLAGS_ADB_ON        0x1
#define FOS_FLAGS_ADB_ROOT      0x2
#define DEV_FLAGS_ADB_USB_ON    0x1000

#define UNLOCK_AUTO_UPDATE_IDME_FOSFLAGS  1

/* Automatically set adb-related flag bits in idme after successful unlock */
int idme_auto_set_after_unlock(void)
{
	int ret;
	char flags_str[19] = { 0 };
	unsigned long long flags = 0;

/* update fos_flags automatically. */
#if UNLOCK_AUTO_UPDATE_IDME_FOSFLAGS
	if (0 != idme_get_var_external("fos_flags", flags_str, 16)) {
		printf("Error getting fos_flags value\n");
		return -1;
	}

	flags = simple_strtoul(flags_str, NULL, 16);

	/* Set adb-on and adb-root bit in fos_flags. */
	flags |= FOS_FLAGS_ADB_ON | FOS_FLAGS_ADB_ROOT;
	memset(flags_str, 0, sizeof(flags_str));
	snprintf(flags_str, sizeof(flags_str), "%llx", flags);
	ret = idme_update_var_ex("fos_flags", flags_str, sizeof(flags_str) - 1);
	if (ret) {
		printf("failed to update fos_flags\n");
		return -1;
	}
	printf("update fos_flags successfully\n");
#endif

	/* Read and update dev_flags automatically. */
	if (0 != idme_get_var_external("dev_flags", flags_str, 16)) {
		printf("Error getting dev_flags value\n");
		return -1;
	}

	flags = simple_strtoul(flags_str, NULL, 16);

	/* Set adb-usb bit in dev_flags. */
	flags |= DEV_FLAGS_ADB_USB_ON;
	memset(flags_str, 0, sizeof(flags_str));
	snprintf(flags_str, sizeof(flags_str), "%llx", flags);
	ret = idme_update_var_ex("dev_flags", flags_str, sizeof(flags_str) - 1);
	if (ret) {
		printf("failed to update dev_flags\n");
		return -1;
	}

	printf("update dev_flags successfully\n");

	return 0;
}

/* Automatically clean adb-related flag bits in idme before relock */
int idme_auto_clean_before_relock(void)
{
	int ret;
	char flags_str[19] = { 0 };
	unsigned long long flags = 0;

/* update fos_flags automatically. */
#if UNLOCK_AUTO_UPDATE_IDME_FOSFLAGS
	if (0 != idme_get_var_external("fos_flags", flags_str, 16)) {
		printf("Error getting fos_flags value\n");
		return -1;
	}

	flags = simple_strtoul(flags_str, NULL, 16);

	/* clear adb-on and adb-root bit in fos_flags. */
	flags &= ~(FOS_FLAGS_ADB_ON | FOS_FLAGS_ADB_ROOT);
	memset(flags_str, 0, sizeof(flags_str));
	snprintf(flags_str, sizeof(flags_str), "%llx", flags);
	ret = idme_update_var_ex("fos_flags", flags_str, sizeof(flags_str) - 1);
	if (ret) {
		printf("failed to update fos_flags\n");
		return -1;
	}
	printf("update fos_flags successfully\n");
#endif

	/* Read and update dev_flags automatically. */
	if (0 != idme_get_var_external("dev_flags", flags_str, 16)) {
		printf("Error getting dev_flags value\n");
		return -1;
	}

	flags = simple_strtoul(flags_str, NULL, 16);

	/* Clear adb-usb bit in dev_flags. */
	flags &= ~DEV_FLAGS_ADB_USB_ON;
	memset(flags_str, 0, sizeof(flags_str));
	snprintf(flags_str, sizeof(flags_str), "%llx", flags);
	ret = idme_update_var_ex("dev_flags", flags_str, sizeof(flags_str) - 1);
	if (ret) {
		printf("failed to update dev_flags\n");
		return -1;
	}

	printf("update dev_flags successfully\n");

	return 0;
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	do_reset(NULL, 0, 0, NULL);
}

static void compl_do_reboot_bootloader(struct usb_ep *ep, struct usb_request *req)
{
	run_command("reboot fastboot", 0);
}


static void cb_reboot(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;

	printf("cmd is %s\n", cmd);

	strsep(&cmd, "-");
	if (!cmd) {
		fastboot_func->in_req->complete = compl_do_reset;
		fastboot_tx_write_str("OKAY");
		return;
	}

	fastboot_func->in_req->complete = compl_do_reboot_bootloader;
	fastboot_tx_write_str("OKAY");
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static void cb_getvar(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
	char *s = NULL;
	char *s1;
	char *s2;
	char *s3;
	size_t chars_left;

	strcpy(response, "OKAY");
	chars_left = sizeof(response) - strlen(response) - 1;

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing variable\n");
		fastboot_tx_write_str("FAILmissing var");
		return;
	}

	if (!strcmp_l1("version", cmd)) {
		strncat(response, FASTBOOT_VERSION, chars_left);
	} else if (!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, U_BOOT_VERSION, chars_left);
	} else if (!strcmp_l1("downloadsize", cmd) ||
		!strcmp_l1("max-download-size", cmd)) {
		char str_num[12];

		sprintf(str_num, "0x%08x", ddr_size_usable(CONFIG_USB_FASTBOOT_BUF_ADDR));
		strncat(response, str_num, chars_left);
	} else if (!strncmp("partition-size", cmd, strlen("partition-size"))) {
		char str_num[20];
		char p_name[MAX_PART_NAME_LEN];
		struct partitions *pPartition;
		uint64_t sz = 0;
		int ret = -1;
		strsep(&cmd, ":");
		if ((cmd == NULL) || (strcmp(cmd, "") == 0)) {
			printf("partition name is NULL\n");
			strcpy(response, "FAILpartition name is NULL");
			fastboot_tx_write_str(response);
			return;
		}
		if (!strncmp("mbr", cmd, strlen("mbr"))) {
			strcpy(response, "FAILVariable not implemented");
		} else {
			pPartition = find_mmc_partition_by_name(cmd);
			if (pPartition) {
				printf("size:%016llx\n", pPartition->size);
				sprintf(str_num, "%016llx", pPartition->size);
			} else if (!strncmp("bootloader-", cmd, strlen("bootloader-"))) {
				strsep(&cmd, "-");
				ret = mmc_boot_size(cmd, &sz);
				printf("ret = %d\n", ret);
				if (ret == 0) {
					printf("size:%016llx\n", sz);
					sprintf(str_num, "%016llx", sz);
				} else {
					printf("get partitize error\n");
					sprintf(str_num, "FAILget partitize error");
				}
			} else {
				printf("find_mmc_partition_by_name fail\n");
				sprintf(str_num, "FAILget fail");
			}
			strncat(response, str_num, chars_left);
		}
	} else if (!strcmp_l1("serialno", cmd)) {
		//s = getenv("serial");
		s = get_usid_string();
#if defined(CONFIG_IDME)
		char buf[24] = "";
		if (!idme_get_var_external("serial", buf, sizeof(buf)))
			s = buf;
#endif
		if (s)
			strncat(response, s, chars_left);
		else
			strncat(response, DEVICE_SERIAL, chars_left);
	} else if (!strcmp_l1("product", cmd)) {
		s1 = DEVICE_PRODUCT;
		strncat(response, s1, chars_left);
	} else if (!strcmp_l1("secure", cmd)) {
		strncat(response,
				(secure_boot_enabled() == true) ? "yes" : "no",
				chars_left);
#if defined(UFBL_FEATURE_UNLOCK)
	} else if (!strcmp_l1("unlock_code", cmd)) {
			unsigned char unlock_code[UNLOCK_CODE_LEN + 1] = {0};
			int code_len = UNLOCK_CODE_LEN;
			char str_num[UNLOCK_CODE_LEN + 1] = {0};
			if (amzn_get_unlock_code(unlock_code, &code_len)) {
				strncat(response, "cannot get unlock code", chars_left);
			} else {
				snprintf(str_num, UNLOCK_CODE_LEN+1, "%s", unlock_code);
				strncat(response, str_num, chars_left);
			}

	} else if (!strcmp_l1("unlock_status", cmd)) {
		char str_num[2];
		snprintf(str_num, sizeof(str_num), "%d", amzn_target_is_unlocked() ? 1 : 0);
		strncat(response, str_num, chars_left);
	} else if (!strcmp_l1("unlock_version", cmd)) {
		strncat(response, "1", chars_left);
#endif
	} else if (!strcmp_l1("slot-count", cmd)) {
		strncat(response, "2", chars_left);
	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		s2 = getenv("slot-suffixes");
		printf("slot-suffixes: %s\n", s2);
		if (s2)
			strncat(response, s2, chars_left);
		else
			strncat(response, "0", chars_left);
	} else if (!strcmp_l1("current-slot", cmd)) {
		s3 = getenv("active_slot");
		printf("active_slot: %s\n", s3);
		strncat(response, s3, chars_left);
	} else if (!strcmp_l1("has-slot:boot", cmd)) {
		if (has_boot_slot == 1) {
			printf("has boot slot\n");
			strncat(response, "yes", chars_left);
		} else
			strncat(response, "no", chars_left);
	} /*else if (!strcmp_l1("partition-type:boot", cmd)) {
		char *s2, *s3;
		s2 = getenv("active_slot");
		printf("active_slot: %s\n", s2);
		s3 = "boot";
		printf("s3  1: %s\n", s3);
		strcat(s3, s2);
		printf("s3  2: %s\n", s3);
		strncat(response, s3, chars_left);
	} */else if (!strcmp_l1("has-slot:system", cmd)) {
		if (has_system_slot == 1) {
			printf("has system slot\n");
			strncat(response, "yes", chars_left);
		} else
			strncat(response, "no", chars_left);
	} else {
		error("unknown variable: %s\n", cmd);
		strcpy(response, "FAILVariable not implemented");
	}
	fastboot_tx_write_str(response);
}

static unsigned int rx_bytes_expected(void)
{
	int rx_remain = download_size - download_bytes;
	if (rx_remain < 0)
		return 0;
	if (rx_remain > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;
	return rx_remain;
}

#define BYTES_PER_DOT	0x20000
static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	unsigned int transfer_size = download_size - download_bytes;
	const unsigned char *buffer = req->buf;
	unsigned int buffer_size = req->actual;
	unsigned int pre_dot_num, now_dot_num;

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	memcpy((void *)CONFIG_USB_FASTBOOT_BUF_ADDR + download_bytes,
	       buffer, transfer_size);

	pre_dot_num = download_bytes / BYTES_PER_DOT;
	download_bytes += transfer_size;
	now_dot_num = download_bytes / BYTES_PER_DOT;

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}

	/* Check if transfer is done */
	if (download_bytes >= download_size) {
		/*
		 * Reset global transfer variable, keep download_bytes because
		 * it will be used in the next possible flashing command
		 */
		download_size = 0;
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;

		sprintf(response, "OKAY");
		fastboot_tx_write_str(response);

		printf("\ndownloading of %d bytes finished\n", download_bytes);
	} else {
		req->length = rx_bytes_expected();
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void cb_download(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	printf("cmd is %s\n", cmd);

	strsep(&cmd, ":");
	download_size = simple_strtoul(cmd, NULL, 16);
	download_bytes = 0;

	printf("Starting download of %d bytes\n", download_size);

	if (0 == download_size) {
		sprintf(response, "FAILdata invalid size");
	} else if (download_size > ddr_size_usable(CONFIG_USB_FASTBOOT_BUF_ADDR)) {
		download_size = 0;
		sprintf(response, "FAILdata too large");
	} else {
		sprintf(response, "DATA%08x", download_size);
		req->complete = rx_handler_dl_image;
		req->length = rx_bytes_expected();
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}
	fastboot_tx_write_str(response);
}

static void do_bootm_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	char boot_addr_start[12];
	char *bootm_args[] = { "bootm", boot_addr_start, NULL };

	puts("Booting kernel..\n");

	sprintf(boot_addr_start, "0x%lx", load_addr);
	do_bootm(NULL, 0, 2, bootm_args);

	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);
}

static void cb_boot(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_bootm_on_complete;
	fastboot_tx_write_str("OKAY");
}

static void do_exit_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	g_dnl_trigger_detach();
}

static void cb_continue(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_exit_on_complete;
	fastboot_tx_write_str("OKAY");
}

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_flash(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;

	printf("cmd is %s\n", cmd);

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name\n");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

#if defined(UFBL_FEATURE_UNLOCK)
	if (!strcmp_l1("unlock", cmd)) {
		if (amzn_verify_unlock((void *)CONFIG_USB_FASTBOOT_BUF_ADDR, download_bytes)) {
			fastboot_tx_write_str("FAILunlock code error");
		} else if (idme_update_var_ex("unlock_code", (void *)CONFIG_USB_FASTBOOT_BUF_ADDR,
					SIGNED_UNLOCK_CODE_LEN)) {
			fastboot_tx_write_str("FAILunlock failed");
		} else {
			idme_auto_set_after_unlock();
			fastboot_tx_write_str("OKAY");
		}

		return;
	}
#endif

	//strcpy(response, "FAILno flash device defined");
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_flash_write(cmd, (void *)CONFIG_USB_FASTBOOT_BUF_ADDR,
			   download_bytes);
#endif
}
#endif

#if defined(CONFIG_IDME)
static void cb_oem_idme(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	char *cmd = req->buf;

	cmd = strstr(cmd, "idme");
	cmd = strstr(cmd, " ");

	if (!fastboot_idme((const char*)cmd))
		strcpy(response, "OKAYidme done");
	else
		strcpy(response, "FAILidme fail");

	fastboot_tx_write_str(response);
}

static inline int is_blank(int c)
{
	return (c == ' ' || c == '\t');
}

static void cmd_oem_bootmode(struct usb_ep *ep, struct usb_request *req)
{
	char response[64] = {0};
	unsigned int bootmode_int = 0;
	char *arg = req->buf;
	int ret = 0;

	arg = strstr(arg, "bootmode");
	arg = strstr(arg, " ");
	if (!arg || *arg == '\0'){
		fastboot_fail("Must specify bootmode");
		return;
	}

	while (*arg && isblank(*arg))
		arg++;

	/* Only accept valid bootmode values */
	bootmode_int = *arg - '0';
	if (bootmode_int > IDME_BOOTMODE_MAX) {
		fastboot_fail("invalid bootmode given");
		return;
	}

	ret = idme_update_var_ex("bootmode", arg, 1);

	if (ret) {
		snprintf(response, 64, "Could not set bootmode, error=%d", ret);
		fastboot_fail(response);
		return;
	}

	snprintf(response, 64, "bootmode set to %c", *arg);
	fastboot_okay(response);
}
/*
 * 'fastboot oem flags <type:> <modifiers> <value1[|value2|value3|...]>'
 *
 * type:
 *  fos_flags:, fos: f: (default if not specified)
 *  dev_flags:, dev:, d:
 *  usr_flags:, usr:, u:
 *
 * modifiers:
 *  + means to OR value with the existing value
 *    (flags = flags | value)
 *  - means to AND the inverse of value with the existing value
 *    (flags = flags & ~value)
 *  = means to set the value (flags = value). If = is not set, it is assumed
 *
 * value:
 * If begins with 0x, it is treated as a hexadecimal value and set directly
 * If the value field is a number, it is converted from decimal to hex and set
 * Multiple values may be ORd together as in value1|value2|value3
 * Symbolic names are supported, and must match the symbolic name used in the header file
 *
 * If no type, modifiers or value is specified, print out the current value.
 * Also, accept the special keyword 'print'
 */

typedef enum {
	FLAGS_CMD_NONE = 0,
	FLAGS_CMD_HELP = 1,
	FLAGS_CMD_PRINT = 2,
	FLAGS_CMD_SET = 3
} flags_cmd_t;

typedef enum {
	FLAGS_TYPE_NONE = 0,
	FLAGS_TYPE_FOS = 1,
	FLAGS_TYPE_DEV = 2,
	FLAGS_TYPE_USR = 3,
	FLAGS_TYPE_ALL = 99
} flags_type_t;

typedef enum {
	FLAGS_MOD_NONE = 0,
	FLAGS_MOD_SET = 1,
	FLAGS_MOD_ADD = 2,
	FLAGS_MOD_REMOVE = 3
} flags_modifier_t;

static inline int ishexprefix(char* s)
{
	return s && *s && ('0' == *s) && ('x' == *(s+1) || 'X' == *(s+1));
}

static inline int issymprefix(char* s)
{
	return s && *s &&
		(!strncmp(s,"FOS_FLAGS_",10) ||
		 !strncmp(s,"DEV_FLAGS_",10) ||
		 !strncmp(s,"USR_FLAGS_",10) );
}

static int hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

static inline unsigned long long atoull(const char *num)
{
	unsigned long long value = 0;

	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++ - '0';
	}

	return value;
}

static inline unsigned long long getnumval(char *s)
{
	if(s && *s) {
		if(ishexprefix(s)) {
			// For hex, normalize to lower case for atoull function
			*(s+1) = tolower(*(s+1));
		}

		return atoull(s);
	}

	return 0;
}

static inline unsigned long long getsymval(char *s)
{
        // TBD
	return 0;
}

#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
static void cmd_oem_standby(struct usb_ep *ep, struct usb_request *req)
{
	char response[64] = {0};
	char cur_flags_str[19] = {0};
	char new_flags_str[19] = {0};
	unsigned int standby_mode_int = 0;
	char *arg = req->buf;
	unsigned long long cur_flags = 0;
	unsigned long long new_flags = 0;
	int ret = 0;

	arg = strstr(arg, "standby");
	arg = strstr(arg, " ");
	if (!arg || *arg == '\0'){
		fastboot_fail("Must specify standby mode");
		return;
	}

	while (*arg && isblank(*arg))
		arg++;

	/* Only accept valid standby values */
	standby_mode_int = *arg - '0';
	if (standby_mode_int > 1)
	{
		fastboot_fail("invalid standy mode given");
		return;
	}

	memset(cur_flags_str, 0, sizeof(cur_flags_str));
	if(0 != idme_get_var_external("dev_flags", cur_flags_str+2, 16)) {
		snprintf(response, 64, "Could not get standby idme , error=%d", ret);
		fastboot_fail(response);
		return;
	}
	/* Need to prefix with 0x for atoull to handle hex. */
	cur_flags_str[0] = '0'; cur_flags_str[1] = 'x';
	cur_flags = atoull(cur_flags_str);
	if (standby_mode_int == 1)
		new_flags = (cur_flags & ~0x4);
	else
		new_flags = (cur_flags | 0x4);
	memset(new_flags_str, 0, sizeof(new_flags_str));
	snprintf(new_flags_str, sizeof(new_flags_str), "%llx", new_flags);
	ret = idme_update_var_ex("dev_flags", new_flags_str, sizeof(new_flags_str)-1);

	if (ret) {
		snprintf(response, 64, "Could not set standby, error=%d", ret);
		fastboot_fail(response);
		return;
	}

	snprintf(response, 64, "standby set to %c", *arg);
	fastboot_okay(response);
}
#endif


static void cmd_oem_flags(struct usb_ep *ep, struct usb_request *req)
{
	/* sizeof(flags) + NULL = 17 */
	char response[64] = { 0 };
	int ret = 0;
	char * target_flags = NULL;
	char new_flags_str[19] = { 0 };
	char cur_flags_str[19] = { 0 };
	unsigned long long cur_flags = 0;
	unsigned long long new_flags = 0;
	unsigned long long value = 0;
	char *arg = req->buf;
	arg = strstr(arg, "flags");
	arg = strstr(arg, " ");

	/* allow modifying to printing usr_flags on locked hw */
	/* Restrict modifying fos_flags or dev_flags on locked hw */

	flags_cmd_t flags_cmd = FLAGS_CMD_PRINT;
	flags_modifier_t flags_modifier = FLAGS_MOD_SET;
	flags_type_t flags_type = FLAGS_TYPE_NONE;

	/* Begin parsing */
	/* Default to a printout, when just oem flags is called */
	if (!arg || !*arg) {
		flags_cmd = FLAGS_CMD_PRINT;
		flags_type = is_locked_production_device()? FLAGS_TYPE_USR : FLAGS_TYPE_FOS;
		flags_modifier = FLAGS_MOD_NONE;
		goto parse_done;
	}

	while (*arg && isblank(*arg))
		arg++;

	if (!*arg)
		goto parse_done;

	/* find type: fos_flags, dev_flags or usr_flags */
	if ((!strncmp(arg, "f:", 2) && (arg+=strlen("f:"))) ||
			(!strncmp(arg, "fos:", 4) && (arg+=strlen("fos:"))) ||
			(!strncmp(arg, "fos_flags:", 10) && (arg+=strlen("fos_flags:"))))
		flags_type = FLAGS_TYPE_FOS;
	else if ((!strncmp(arg, "d:", 2) && (arg+=strlen("d:"))) ||
			(!strncmp(arg, "dev:", 4) && (arg+=strlen("dev:"))) ||
			(!strncmp(arg, "dev_flags:", 10) && (arg+=strlen("dev_flags:"))))
		flags_type = FLAGS_TYPE_DEV;
	else if ((!strncmp(arg, "u:", 2) && (arg+=strlen("u:"))) ||
			(!strncmp(arg, "usr:", 4) && (arg+=strlen("usr:"))) ||
			(!strncmp(arg, "usr_flags:", 10) && (arg+=strlen("usr_flags:"))))
		flags_type = FLAGS_TYPE_USR;

	/* Find modifier +, -, = (= default) */
	while (*arg && is_blank(*arg))
		arg++;
	if(!*arg)
		goto parse_done;

	switch (*arg) {
	case '+':
		flags_modifier = FLAGS_MOD_ADD;
		arg++;
		break;
	case '-':
		flags_modifier = FLAGS_MOD_REMOVE;
		arg++;
		break;
	case '=':
		flags_modifier = FLAGS_MOD_SET;
		arg++;
		break;
	}

	/* Find value, or special case */
	while (*arg && isblank(*arg))
		arg++;
	if (!*arg)
		goto parse_done;

	if (!strncmp(arg,"print", 5) ||
			!strncmp(arg,"PRINT", 5)) {
		arg+=strlen("print");
		flags_cmd = FLAGS_CMD_PRINT;
		flags_modifier = FLAGS_MOD_NONE;
		while (*arg && isblank(*arg)) arg++;

		if (FLAGS_TYPE_NONE == flags_type)
			flags_type = FLAGS_TYPE_FOS;

		if (is_locked_production_device() || ('u' == *arg) || ('U' == *arg))
			flags_type = FLAGS_TYPE_USR;
		else if (('f' == *arg) || ('F' == *arg))
			flags_type = FLAGS_TYPE_FOS;
		else if (('d' == *arg) || ('D' == *arg))
			flags_type = FLAGS_TYPE_DEV;

		goto parse_done;
	} else if (!strcmp(arg,"help") ||
			!strcmp(arg,"HELP") ||
			!strcmp(arg,"?")) {
		flags_cmd = FLAGS_CMD_HELP;
		flags_modifier = FLAGS_MOD_NONE;
		flags_type = FLAGS_TYPE_NONE;
		goto parse_done;
	} else {
		/*
		 * see if we have a value string
		 * while(*arg && isblank(*arg)) arg++; if(!*arg) goto parse_done;
		 */
		value = 0;
		char* cur_ptr = NULL;
		while (*arg) {
			/* skip whitespace */
			while(*arg && isblank(*arg)) arg++;

			cur_ptr = arg;
			while (*arg && *arg != '|' && !isblank(*arg)) arg++;
			if (*arg) {
				*arg = 0;
				/* Prepare for next round */
				arg++;
				while (*arg && (isblank(*arg) || '|' == *arg))
					arg++;
			}

			flags_cmd = FLAGS_CMD_SET;
			if (issymprefix(cur_ptr))
				value |= getsymval(cur_ptr);
			else
				value |= getnumval(cur_ptr);
		}
	}

parse_done:
	if (is_locked_production_device() && flags_cmd >= FLAGS_CMD_SET) {
		fastboot_fail("oem flags command is restricted on locked hw");
		return;
	}

	if (FLAGS_CMD_SET == flags_cmd && FLAGS_TYPE_NONE == flags_type)
		flags_type =  FLAGS_TYPE_FOS;

	switch (flags_type) {
	case FLAGS_TYPE_DEV:
		target_flags = "dev_flags";
		break;
	case FLAGS_TYPE_USR:
		target_flags = "usr_flags";
		break;
	default:
		target_flags = "fos_flags";
		break;
	}

	cur_flags = 0;
	memset(cur_flags_str, 0, sizeof(cur_flags_str));
	if(0 != idme_get_var_external(target_flags, cur_flags_str+2, 16)) {
		printf("Error getting %s value\n", target_flags);
		cur_flags_str[0]=0;
	}
	/* Need to prefix with 0x for atoull to handle hex. */
	cur_flags_str[0] = '0'; cur_flags_str[1] = 'x';
	cur_flags = atoull(cur_flags_str);

	switch(flags_cmd) {
	case FLAGS_CMD_PRINT:
		if ( FLAGS_TYPE_FOS ==  flags_type ) {
			int size_needed = fos_flags_to_str( cur_flags, NULL, 0 );
			char* b = (char*)malloc(size_needed);
			fos_flags_to_str( cur_flags, b, size_needed );
			printf("%s\n", b);
			free(b);
		}
		snprintf(response, 64, "%s is 0x%llx\n", target_flags, cur_flags);
		fastboot_okay(response);
		return;
		break;
	case FLAGS_CMD_SET:
		switch (flags_modifier) {
			case FLAGS_MOD_ADD:
				new_flags = (cur_flags | value);
				break;
			case FLAGS_MOD_REMOVE:
				new_flags = (cur_flags & ~value);
				break;
			default:
				new_flags = value;
				break;
		}

		memset(new_flags_str, 0, sizeof(new_flags_str));
		snprintf(new_flags_str, sizeof(new_flags_str), "%llx", new_flags);

		ret = idme_update_var_ex(target_flags, new_flags_str, sizeof(new_flags_str)-1);
		if (ret) {
			snprintf(response, 64, "could not set %s, error=%d", target_flags, ret);
			fastboot_fail(response);
			return;
		}

		snprintf(response, 64, "%s set to %s", target_flags, new_flags_str);
		printf("%s\n", response);
		fastboot_okay(response);
		return;
		break;
	default:
		fastboot_okay("oem flags [<type> <modifier>] <value>");
		break;
	} /* switch(flags_cmd) */

	fastboot_fail("Unhandled");

}
#endif

static void cb_flashall(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;

	printf("cmd is %s\n", cmd);

	//strcpy(response, "FAILno flash device defined");

#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_flash_write(cmd, (void *)CONFIG_USB_FASTBOOT_BUF_ADDR,
			   download_bytes);
#endif
}

static void cb_erase(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;

	printf("cmd is %s\n", cmd);

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name\n");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	//strcpy(response, "FAILno erase device defined");
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_erase_write(cmd, (void *)CONFIG_USB_FASTBOOT_BUF_ADDR);
#endif
}

static void cb_devices(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	char *cmd = req->buf;

	printf("cmd is %s\n", cmd);

	strcpy(response, "AMLOGIC");

	fastboot_tx_write_str(response);
}

struct cmd_dispatch_info {
	char *cmd;
	void (*cb)(struct usb_ep *ep, struct usb_request *req);
};

#if defined(UFBL_FEATURE_UNLOCK)
static void cb_oem_relock(struct usb_ep *ep, struct usb_request *req)
{
	char code[SIGNED_UNLOCK_CODE_LEN] = {0};
	char response[RESPONSE_LEN] = {0};

	if (idme_update_var_ex("unlock_code", code, sizeof(code))) {
		strcpy(response, "FAILrelock failed");
	} else {
		idme_auto_clean_before_relock();
		strcpy(response, "OKAY");
	}

	fastboot_tx_write_str(response);
	return;
}
#endif

static const struct cmd_dispatch_info cmd_dispatch_info[] = {
	{
		.cmd = "reboot",
		.cb = cb_reboot,
	}, {
		.cmd = "getvar:",
		.cb = cb_getvar,
	}, {
		.cmd = "download:",
		.cb = cb_download,
	}, {
		.cmd = "boot",
		.cb = cb_boot,
	}, {
		.cmd = "continue",
		.cb = cb_continue,
	},
#ifdef CONFIG_FASTBOOT_FLASH
	{
		.cmd = "flash",
		.cb = cb_flash,
	},
#endif
	{
		.cmd = "update",
		.cb = cb_download,
	},
	{
		.cmd = "flashall",
		.cb = cb_flashall,
	},
	{
		.cmd = "erase",
		.cb = cb_erase,
	},
	{
		.cmd = "devices",
		.cb = cb_devices,
	},
	{
		.cmd = "reboot-bootloader",
		.cb = cb_reboot,
	},
#if defined(CONFIG_IDME)
	{
		.cmd = "oem idme",
		.cb = cb_oem_idme,
	},
	{
		.cmd = "oem bootmode",
		.cb = cmd_oem_bootmode,
	},
	{
		.cmd = "oem flags",
		.cb = cmd_oem_flags,
	},
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
	{
		.cmd = "oem standby",
		.cb = cmd_oem_standby,
	},
#endif
#endif
#if defined(UFBL_FEATURE_UNLOCK)
	{
		.cmd = "oem relock",
		.cb = cb_oem_relock,
	},
#endif
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req) = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(cmd_dispatch_info); i++) {
		if (!strcmp_l1(cmd_dispatch_info[i].cmd, cmdbuf)) {
			func_cb = cmd_dispatch_info[i].cb;
			break;
		}
	}
	if (!func_cb) {
		error("unknown command: %s\n", cmdbuf);
		fastboot_tx_write_str("FAILunknown command");
#if defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
	} else if (is_restricted_command_on_locked_hw(cmdbuf)) {
		error("I'm sorry, but command %s is blocked at fastboot\n",
				cmd_dispatch_info[i].cmd);
		fastboot_tx_write_str("FAILblocked command");
#endif
	} else {
		if (req->actual < req->length) {
			u8 *buf = (u8 *)req->buf;
			buf[req->actual] = 0;
			func_cb(ep, req);
		} else {
			error("buffer overflow\n");
			fastboot_tx_write_str("FAILbuffer overflow");
		}
	}

	if (req->status == 0) {
		*cmdbuf = '\0';
		req->actual = 0;
		usb_ep_queue(ep, req, 0);
	}
}
#if defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
void amzn_extends_fastboot_whitelist(const char* const **list, int *length) {
	if (list == NULL || length == NULL) return;
	static const char* const whitelist[] = {
		"oem standby",
	};
	*list = whitelist;
	*length = sizeof(whitelist) / sizeof(whitelist[0]);
}
#endif
#endif

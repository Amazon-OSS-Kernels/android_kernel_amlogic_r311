

#ifndef UBOOT_LOG_H
#define UBOOT_LOG_H

#if defined(CONFIG_UBOOT_LOGGER)
#ifndef UBOOT_LOG_BUF_SIZE
#define UBOOT_LOG_BUF_SIZE    (1024*16) /* align on 4k */
#endif

#define UBOOT_LOG_COOKIE    0x474f4c55 /* "KLOG" in ASCII */
#define UBOOT_LOG_DATA_LENGTH    (1024*16-16)
struct uboot_log {
	struct uboot_log_header {
		unsigned cookie;
		unsigned max_size;
		unsigned size_written;
		unsigned idx;
	} header;
	char data[UBOOT_LOG_DATA_LENGTH];
};

#endif

#endif

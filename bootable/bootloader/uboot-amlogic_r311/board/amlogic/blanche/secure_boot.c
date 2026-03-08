/*
 * secure_boot.c
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#include <asm-generic/gpio.h>
#include <asm/arch/secure_apb.h>
#include <asm/io.h>
#include <common.h>
#include <ctype.h>
#include "amzn_lockdown.h"
#include "amzn_secure_boot.h"
#if defined(UFBL_FEATURE_UNLOCK)
#include <amzn_unlock.h>
#include <u-boot/sha256.h>
#endif
#include <amzn_secure_flashing.h>
/* 4th & 28th bits indicate secure boot status */
#define SECURE_CHIP_BITS	((1 << 4) | (1 << 28))

bool secure_boot_enabled(void)
{
	unsigned int reg_value;

	reg_value = *((volatile unsigned int *) AO_SEC_SD_CFG10);
	return (((reg_value & SECURE_CHIP_BITS) == SECURE_CHIP_BITS) ? true : false);
}

const char *amzn_target_device_name(void)
{
	static char target_name[16]={0};
	int i=0;
	strncpy(target_name,CONFIG_DEVICE_PRODUCT,strlen(CONFIG_DEVICE_PRODUCT));
	while(i<strlen(target_name)){
		target_name[i]=tolower(target_name[i]);
		i++;
	}
	printf("target_device_name is %s\n",target_name);
	return target_name;
//	return "blanche";
}

int amzn_target_device_type(void)
{
	unsigned int gpioz_19;
	int device_type, gpio_val = 0;

	/* HVT device */
	if (!secure_boot_enabled())
		return AMZN_ENGINEERING_DEVICE;
#if defined UBOOT_TARGET_PRODUCT_NAME_ABC
	/* Is anti-rollback enabled? */
	if (query_efuse_status("ARB") == 1){
		device_type = AMZN_PRODUCTION_DEVICE;
	}
	else {
		device_type = AMZN_ENGINEERING_DEVICE;;
	}

#else
	if (query_efuse_status("ARB") == 1)
		return AMZN_PRODUCTION_DEVICE;
	
	/* set pinmux --gpio mode */
	clrbits_le32(P_PERIPHS_PIN_MUX_4, (1<<4));
	clrbits_le32(P_PERIPHS_PIN_MUX_3, (1<<1) | (1<<22) | (1<<27) | (1<<28));

	gpio_lookup_name("gpioz_19", NULL, NULL, &gpioz_19);
	gpio_request(gpioz_19, "cmd_gpio");
	gpio_direction_input(gpioz_19);

	gpio_val = gpio_get_value(gpioz_19);
	if (gpio_val)
		device_type = AMZN_ENGINEERING_DEVICE;
	else
		device_type = AMZN_PRODUCTION_DEVICE;
#endif

	printf("%s Device!!\n", device_type == AMZN_ENGINEERING_DEVICE? \
			"Engineering": "Production");

	return device_type;
}

#if defined(UFBL_FEATURE_UNLOCK)
#define CHIPID_UPPER		(6)
#define CHIPID_LOWER		(7)
#define CHIPID_BUF_SIZE		(16)
#define HASH_BUF_SIZE		(32)

int amzn_get_unlock_code(unsigned char *code, unsigned int *len)
{
	sha256_context ctx;
	uint8_t buff[CHIPID_BUF_SIZE] = {0};
	uint8_t hash[HASH_BUF_SIZE] = {0};

	if (!code || !len || *len < (16 + 1))
		return -1;

	if (get_chip_id(&buff[0], sizeof(buff)))
		return -1;
	/**
	 * To sync with Amazon serial number from kernel's /proc/cpuinfo,
	 * the unlock_code is low 64 bit of sha256(SoC Chipid 128bits).
	 */
	sha256_starts(&ctx);
	sha256_update(&ctx, &buff[0], sizeof(buff));
	sha256_finish(&ctx, &hash[0]);
	u32 *hashcode = (u32 *) &hash[0];
	snprintf(code, CHIPID_BUF_SIZE+1, "%08x%08x",be32_to_cpu(hashcode[CHIPID_UPPER]),
			be32_to_cpu(hashcode[CHIPID_LOWER]));

	*len = 16;
	return 0;
}

const unsigned char *amzn_get_unlock_key(unsigned int *key_len)
{
	/* TODO: replace by Blanche */
	/* unlock_abc123.pub */
	static const unsigned char unlock_key[] =
		"\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01"
		"\x01\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01"
		"\x00\xcb\x75\xfe\x53\xc1\xab\xd3\x5a\x8b\x5d\xca\x5b\x57\x76\x2f"
		"\x8c\xcc\x5a\xfc\x0e\xd5\xbd\x2e\xa9\x37\x10\xc1\x65\xd9\xea\x81"
		"\xe3\x83\x61\x47\xe1\xe2\x37\xa1\x3e\xd1\xca\x87\x6f\xeb\xd6\x97"
		"\x30\x4d\xb6\x25\xc7\xf5\xb9\x4f\xf4\x33\x84\xec\x96\xee\xe1\x74"
		"\x21\xbb\xea\xfe\xe5\xb8\x07\x33\x00\x05\x7d\x76\xd5\x84\x30\xd6"
		"\x8a\x06\x82\x4c\x11\xec\x1c\x74\xf8\x74\x6c\xd9\xe9\x6c\x1e\xe9"
		"\x8a\x32\x15\x3c\x62\xbb\x05\x78\x84\x89\x2c\x13\xd9\x28\x29\x91"
		"\xee\x12\xc8\x88\x1f\x27\xa5\xef\x94\x92\x51\xec\x42\xb5\x4b\x6c"
		"\x53\x57\x2c\x17\xe6\xf4\x27\x9f\x9d\x7c\x4c\x1b\x75\x79\xb7\x1a"
		"\x6f\x5a\xae\x1a\x41\x74\xb6\xdc\x45\xfb\x23\x46\xc2\x91\xea\x7e"
		"\xd0\x41\x3e\xca\x39\xf1\x86\xe4\x98\x1f\x38\xbe\xe4\x67\x68\x87"
		"\x4c\x99\x39\x27\xd3\x32\xba\x10\x59\x56\xd2\xef\x8d\xde\x6e\xc6"
		"\x69\xb2\x46\x5d\xb0\x21\xc1\x38\xd9\xb4\x16\x4a\x1b\xf4\x5d\x7a"
		"\xba\xc6\x29\x4d\x8e\x59\x9e\x98\xa1\x5b\x06\xc0\xcc\x5b\x61\x32"
		"\x22\x81\x49\xcb\x98\xd8\x45\x6e\x29\x0b\x37\x61\x22\xbd\x79\xce"
		"\xf8\xa2\xf7\x3c\x46\x9c\x92\x6c\x42\x05\x65\xce\x7e\xd6\x46\x21"
		"\xe5\x02\x03\x01\x00\x01"
		;

	const int unlock_key_size = sizeof(unlock_key);
	if (!key_len)
		return NULL;

	*key_len = unlock_key_size;

	return unlock_key;
}

#ifdef UFBL_FEATURE_SECURE_FLASHING
static unsigned int get_random_number(int s)
{
	unsigned int seed = (unsigned int)get_timer(0) + s;
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);
	return seed;
}

int amzn_get_sec_flashing_code(unsigned char *code, unsigned int *len)
{
	static unsigned char sec_flashing_code[SEC_FLASH_CODE_LEN + 1] = {0};
	static unsigned char code_generated = 0;
	unsigned int unlock_code_len = SEC_FLASH_CODE_LEN;
	unsigned int rand1, rand2;

	if (!code || !len || *len < SEC_FLASH_CODE_LEN)
		return -1;

	if (!code_generated) {
		if(amzn_get_unlock_code(sec_flashing_code, &unlock_code_len)){
			return -1;
		}
		rand1 = get_random_number(0x1AB126);
		rand2 = get_random_number(rand1);
		sprintf(&sec_flashing_code[16], "%08x%08x", rand1, rand2);
		code_generated = 1;
	}
	memcpy(code, sec_flashing_code, SEC_FLASH_CODE_LEN);
	*len = SEC_FLASH_CODE_LEN;

	return 0;
}

const unsigned char *amzn_get_sec_flashing_root_pubkey(unsigned int *key_len)
{
	static const unsigned char root_key[] =
		"\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01"
		"\x01\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01"
		"\x00\xb9\x20\xa0\x41\x68\x31\x06\xf4\x97\x32\x0d\xfc\x3a\x6c\x6a"
		"\xe9\x41\x6e\xfd\x57\x47\xd3\xdc\xef\xd7\x75\x24\x79\x33\x39\x71"
		"\x02\xd8\x72\x37\xd0\xdc\xc4\xed\x3d\x40\x6a\x20\xfa\xc7\x3f\x8e"
		"\x61\x81\xee\xff\x83\xaf\xbe\xb4\x51\xd8\xd2\x01\x42\xd5\x16\xda"
		"\x57\x12\x49\xaa\x3b\x50\xc7\x7e\xec\x47\x0b\x96\x31\xde\xa7\x4a"
		"\x9d\x7f\x7a\x44\xb3\xc2\x62\x8c\xa5\xe0\x0d\x48\xd9\x50\xa9\x69"
		"\xdc\x29\x42\x22\x33\xbb\xb0\x87\xfa\x51\x27\xd5\xf7\x11\x0c\x17"
		"\xbc\xe5\x5c\xa5\x60\x41\xd7\x07\xc0\xc2\x23\x65\x10\xb0\xc2\xa9"
		"\x12\xc4\x56\x80\xb9\xab\xf9\x1a\x89\xf0\x69\x98\xb3\xce\x9d\x22"
		"\x5a\xdf\xf2\x72\xf1\x93\x6e\xf9\xf4\x43\x87\xd0\x7c\xea\x21\x1b"
		"\xfd\xd9\xeb\xda\xba\x1c\x2a\x40\x3b\x3f\x22\xa8\xbc\x18\x5e\x85"
		"\x00\x84\xad\xb5\x88\xd1\x7f\x3d\x96\x73\x9a\x04\x78\xe5\x10\x5f"
		"\xdf\xed\x8c\xe2\x41\x8f\x21\x64\xf7\x54\xa7\xf2\xec\xc1\xe3\x09"
		"\x6e\x5f\xca\xdb\x78\x37\x29\xc0\x2a\xe1\xc5\x77\x32\xce\x5a\x0d"
		"\x4a\x30\xfd\x27\x8d\xa6\x11\x87\x62\xf6\x43\x44\xa7\x3a\xc6\x80"
		"\x03\xfc\x61\xfc\x6d\xae\xc5\x55\xcf\x5c\xee\x04\x24\x31\xb6\x7a"
		"\x5b\x02\x03\x01\x00\x01";
	const int key_size = sizeof(root_key);
	if (!key_len)
		return NULL;
	*key_len = key_size;
	return root_key;
}
#endif /* UFBL_FEATURE_SECURE_FLASHING */

#endif

#include <common.h>
#include <command.h>
#include <amzn_unlock.h>

#ifdef CONFIG_CMD_SECFLASH
int do_secflash(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	int verify_ret = 0;
	unsigned char *b64;
	unsigned int out_len;
	unsigned char sec_flashing_code[UNLOCK_CODE_LEN + 1] = {0};
	unsigned int code_len = sizeof(sec_flashing_code);

	if (argc < 2) {
		ret = -1;
		goto done;
	}

	if (!strncmp(argv[1], "getcode", 6)) {
		if (amzn_get_sec_flashing_code(sec_flashing_code, &code_len)) {
			ret = -2;
			goto done;
		} else {
			printf("%s\n", sec_flashing_code);
		}
	} else if (!strncmp(argv[1], "setcode", 7)) {
		if(argc < 3) {
			ret = -3;
			goto done;
		}
		b64 = (unsigned char *)argv[2];
		out_len = strlen(b64);
		if(amzn_base64_decode(b64, out_len, b64, &out_len)) {
			ret = -4;
			goto done;
		} else {
			if(amzn_verify_sec_flashing_code(b64, out_len)) {
				ret = -5;
				goto done;
			} else {
				amzn_set_sec_flashing_signed_code(b64, out_len);
			}
		}
	} else if (!strncmp(argv[1], "setcert", 7)) {
		if(argc < 3) {
			ret = -6;
			goto done;
		}
		b64 = (unsigned char *)argv[2];
		out_len = strlen(b64);
		if (amzn_base64_decode(b64, out_len, b64, &out_len)) {
			printf("decode error !\n");
			ret = -7;
			goto done;
		}

		if (amzn_set_sec_flashing_cert(b64, out_len)) {
			ret = -8;
			goto done;
		}

		verify_ret = amzn_verify_sec_flashing_cert(1);

		if (verify_ret) {
//#if  defined UBOOT_TARGET_PRODUCT_NAME_abc123
#if  1
			verify_ret = amzn_verify_sec_flashing_cert(1);
			if (verify_ret) {
				ret = -9;
				goto done;
			}
#else
			ret = -9;
			goto done;
#endif
		}
	} else {
		ret = -10;
		goto done;
	}
	ret = 0;
done:
	if (ret)
		printf("do_secflash fail: %d\n", ret);
	else
		printf("do_secflash pass\n");

	return 0;
}

U_BOOT_CMD(
	secflash, CONFIG_SYS_MAXARGS, 1, do_secflash,
		" secure flashing",
		" <getcode>\n"
		"secflash setcode <signed_code>\n"
		"secflash setcert <signed_cert>\n"
);
#endif

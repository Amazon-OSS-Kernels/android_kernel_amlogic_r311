/*
 * (C) Copyright 2018
 * Zhigang.Yu@amlogic.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Uboot update
 */
#include <common.h>
#include <command.h>
#include <s_record.h>
#include <net.h>
#include <ata.h>
#include <asm/io.h>
#include <part.h>
#include <fat.h>
#include <fs.h>

#ifdef CONFIG_UBOOT_USB_UPDATE

#define MAX_IDME_NUM (32)
#define NAME_MAX    (32)
#define VALUE_MAX    (1024)
#define MAX_LINE_BUFF (NAME_MAX+VALUE_MAX)

int num = 0;
char partition[20][32];
int narb = 0;

typedef struct {
    char name[NAME_MAX];
    char value[VALUE_MAX];
} idme_t;

int nidme = 0;
idme_t idme_data[MAX_IDME_NUM];
uint32_t mvn_1, mvn_2;
idme_t arb_version[MAX_IDME_NUM];

static u32 fb_width;
static u32 fb_height;
static u32 display_bpp;
static unsigned char *fb_addr;
extern unsigned long get_fb_addr(void);
extern void read_arb_version(uint32_t *mvn_1_p, uint32_t *mvn_2_p);

#define color_red (0xff << 16)
#define color_green (0xff << 8)
#define color_blue (0xff << 0)
#define color_white (color_red | color_green | color_blue)

static int show_color_block(u32 color, int pos_x, int pos_y, int w, int h)
{
#if 1
    int i, j;
    unsigned char *fbp = NULL;
    int byte_per_pixel;

    if (!fb_addr) {
        printf("framebuffer wasn't initialized\n");
        goto error;
    }

    if (pos_x < 0 || w < 0 ||
            pos_y < 0 || h < 0 ||
            (pos_x + w) > fb_width ||
            (pos_y + h) > fb_height) {
        printf("position is out of range, image upgrade failed\n");
        goto error;
    }

    byte_per_pixel = display_bpp / 8;
    fbp = fb_addr + (fb_width * pos_y + pos_x) * byte_per_pixel;

    for (i = 0; i < h ; i++) {
        for (j = 0; j < w; j++) {
            *(fbp + 0) = color & 0xff;
            *(fbp + 1) = (color >> 8) & 0xff;
            *(fbp + 2) = (color >> 16) & 0xff;
            fbp += byte_per_pixel;
        }
        fbp += (fb_width - w) * byte_per_pixel;
    }

    flush_cache(fb_addr, fb_width * fb_height * byte_per_pixel);

    return 0;
error:
    return -1;
#else
	return 0;
#endif
}

static int show_flash_progress(int cur_step, int total_steps, int result)
{
#if 1
    int x, y, w, h, color;
    const int progress_bar_x = 100;
    const int progress_bar_y = 800;
    const int progress_bar_w = fb_width - 200;
    const int progress_bar_h = 100;

    if (cur_step < 0 || total_steps < 0 || cur_step > total_steps ||
            (cur_step != 0 && total_steps == 0))
        goto error;

    x = progress_bar_x;
    y = progress_bar_y;
    h = progress_bar_h;
    w = !(cur_step | total_steps) ? progress_bar_w :
        progress_bar_w * cur_step / total_steps;
    color = !(cur_step | total_steps) ? color_white :
        result ? color_red : color_green;

    show_color_block(color, x, y, w, h);

    return 0;
error:
    return -1;
#else
	return 0;
#endif
}

static int update_ui_init(void)
{
#if 1
    char *str = NULL;

    fb_addr = (unsigned char *)get_fb_addr();

    str = getenv("fb_width");
    fb_width = str ? simple_strtoul(str, NULL, 10) : 0;

    str = getenv("fb_height");
    fb_height = str ? simple_strtoul(str, NULL, 10) : 0;

    str = getenv("display_bpp");
    display_bpp = str ? simple_strtoul(str, NULL, 10) : 0;
    if (display_bpp != 24) {
        printf("only 24bpp is supported now\n");
        goto error;
    }

    if (!(fb_width && fb_width && display_bpp && fb_addr))
        goto error;

    return 0;
error:
    return -1;
#else
	return 0;
#endif
}


void init_param(void) {
    num = 0;
    memset(partition, 0, sizeof(partition));

    nidme = 0;
    memset(idme_data, 0, MAX_IDME_NUM*sizeof(idme_t));
}

void strtrim(char *strIn, char *strOut){

    int i, j ;
    i = 0;
    j = strlen(strIn) - 1;
    while(strIn[i] == ' ')
        ++i;

    while(strIn[j] == ' ')
        --j;
    strncpy(strOut, strIn + i , j - i + 1);
    strOut[j - i + 1] = '\0';
}

//eg: board_id="ffffff00000000aa"
//then: idme board_id "ffffff00000000aa"
int idme_parse(const char *idme) {

    //max support 1024+32
    char buff[MAX_LINE_BUFF] = {0};
    memcpy(buff, idme, MAX_LINE_BUFF - 1);

    char *pb = strstr(buff, "=");
    if (!pb) {
        //not idme
        return -1;
    }

    int len = strlen(buff);
    //for standard format eg: a="1", board_id="ffffff00000000aa"
    //so size must >= 5
    if (len < 5) {
        printf("wrong idme format config!\n");
        return -1;
    }
    int offset = pb - buff;
    if (nidme > MAX_IDME_NUM-1) {
        printf("max value(%d) support idme set!\n", MAX_IDME_NUM);
        return -1;
    }

    if (offset < 1 || offset > NAME_MAX-1 ) {
        printf("idme name wrong!\n");
        return -1;
    }

    if (len-offset-3 < 1 || len-offset-3 > VALUE_MAX-1) {
        printf("idme value wrong!\n");
        return -1;
    }

    //get idme name and value
    memcpy(idme_data[nidme].name, buff, offset);
    memcpy(idme_data[nidme].value, buff+offset+2, len-offset-3);
    nidme++;
    return 0;
}

int arb_parse(const char *arb_buf) {
	char buff[MAX_LINE_BUFF] = {0};
	memcpy(buff, arb_buf, MAX_LINE_BUFF - 1);
	char *pb = strstr(buff, "=");
	if (!pb) {
		//err config
		return -1;
	}

	int len = strlen(buff);
	int offset = pb - buff;
	if (narb > MAX_IDME_NUM-1) {
		printf("max value(%d) support arb version set!\n", MAX_IDME_NUM);
		return 0;
	}

	//get arb version name and value
	memcpy(arb_version[narb].name, buff, offset);
	memcpy(arb_version[narb].value, buff+offset+1, len-offset-1);

	narb++;
	return 0;
}

int arb_check_version(void)
{
	int i = 0, ret = 0;
	uint32_t bl2_flags = 0, fip_flags = 0, bl30_flags = 0, bl31_flags = 0, bl32_flags = 0, bl33_flags = 0;
	ulong addr = 0;
	int filesize = 0;
	char out[MAX_LINE_BUFF] = {0};
	uint32_t mvn_1 = 0, mvn_2 = 0;
	printf("---before read_arb_version--\n");
	read_arb_version(&mvn_1, &mvn_2);
	printf("---after read_arb_version--\n");
	uint32_t bl33 = (mvn_1) & 0xff;
	uint32_t bl32 = (mvn_1 >> 8) & 0xff;
	uint32_t bl31 = (mvn_1 >> 16) & 0xff;
	uint32_t bl30 = (mvn_1 >> 24) & 0xff;
	uint32_t fip  = (mvn_2 >> 16) & 0xff;
	uint32_t bl2  = (mvn_2 >> 24) & 0xff;

	setenv("arbaddr", "0x3090000");
	ret = run_command("fatload usb 0 ${arbaddr} arb_version.txt", 1);
	if (ret < 0)
        	return -1;
	filesize = (int)getenv_hex("filesize", 0);

	//malloc buf for read arb_version.txt
	char *buf = (char *)malloc(256);
	if (buf == NULL) {
		printf("malloc buffer failed!\n");
		return -1;
	}

	//get arb_version.txt
	addr = simple_strtoul(getenv("arbaddr"), NULL, 16);
	memset(buf, 0, 256);
	memcpy(buf, (char *)addr, 256);
	buf[filesize]= '\0';


	//parse arb_version.txt
	char *temp = strtok(buf,"\n");

	while(temp)
	{
		memset(out, 0, MAX_LINE_BUFF);
		strtrim(temp, out);

		if (!arb_parse(out)) {
			temp = strtok(NULL,"\n");
			continue;
		}
		temp = strtok(NULL,"\n");
	}

	free(buf);

	for (i = 0; i < narb; i++)
	{
		if(strncmp("BL2", arb_version[i].name, strlen("BL2"))==0)
		{
			bl2_flags =1;
			printf("unsigned int BL2=%d\n", simple_strtoul(arb_version[i].value, NULL, 0));
			if(bl2 > simple_strtoul(arb_version[i].value, NULL, 0))
			{
				printf("%s version is low ...\n",arb_version[i].value);
				return -1;
			}
		} else if (strncmp("FIP", arb_version[i].name, strlen("FIP"))==0) {
			fip_flags =1;
			printf("unsigned int FIP=%d\n", simple_strtoul(arb_version[i].value, NULL, 0));
			if(fip > simple_strtoul(arb_version[i].value, NULL, 0))
			{
				printf("%s version is low ...\n",arb_version[i].value);
				return -1;
			}
		} else if (strncmp("BL30", arb_version[i].name, strlen("BL30"))==0) {
			bl30_flags =1;
			printf("unsigned int BL30=%d\n", simple_strtoul(arb_version[i].value, NULL, 0));
			if(bl30 > simple_strtoul(arb_version[i].value, NULL, 0))
			{
				printf("%s version is low ...\n",arb_version[i].value);
				return -1;
			}
		} else if (strncmp("BL31", arb_version[i].name, strlen("BL31"))==0) {
			bl31_flags =1;
			printf("unsigned int BL31=%d\n", simple_strtoul(arb_version[i].value, NULL, 0));
			if(bl31 > simple_strtoul(arb_version[i].value, NULL, 0))
			{
				printf("%s version is low ...\n",arb_version[i].value);
				return -1;
			}
		}
		else if (strncmp("BL32", arb_version[i].name, strlen("BL32"))==0) {
			bl32_flags =1;
			printf("unsigned int BL32=%d\n", simple_strtoul(arb_version[i].value, NULL, 0));
			if(bl32 > simple_strtoul(arb_version[i].value, NULL, 0))
			{
				printf("%s version is low ...\n",arb_version[i].value);
				return -1;
			}
		} else if (strncmp("BL33", arb_version[i].name, strlen("BL33"))==0) {
			bl33_flags =1;
			printf("unsigned int BL33=%d\n", simple_strtoul(arb_version[i].value, NULL, 0));
			if(bl33 > simple_strtoul(arb_version[i].value, NULL, 0))
			{
				printf("%s version is low ...\n",arb_version[i].value);
				return -1;
			}
		}
	}
       if (bl2_flags && fip_flags && bl30_flags && bl31_flags && bl32_flags && bl33_flags)
               return 0;
       else {
               printf("check arb_version.txt fail!\n");
               return -1;
       }
}

static int idme_set( void ) {
    int i = 0, ret = 0;
    char cmd[MAX_LINE_BUFF+8];
    for (i=0; i<nidme; i++) {
        memset(cmd, 0, MAX_LINE_BUFF+8);
        sprintf(cmd, "idme %s %s", idme_data[i].name, idme_data[i].value);
        printf("cmd:%s\n", cmd);
        ret += run_command(cmd, 1);
    }
	return ret;
}

int image_check(const char *file) {
    int ret = file_exists("usb", "0", file, FS_TYPE_FAT);
    return ret;
}

extern bool secure_boot_enabled(void);

void image_update(void ) {
    int index = 0, ret = 0;
    char cmd[128] = {0};

    idme_set();

    //update image
    for (index=0; index<num; index++) {
        if (!strcmp(partition[index], "dt")) {//update mbr
            sprintf(cmd, "fatload usb 0 ${loadaddr} %s.img", partition[index]);
            ret += run_command(cmd, 1);
            sprintf(cmd, "store mbr ${loadaddr}");
            ret += run_command(cmd, 1);
        } else if (!strcmp(partition[index], "bootloader")) {//update bootloader
            sprintf(cmd, "store erase partition misc");
            ret += run_command(cmd, 1);
            if (true == secure_boot_enabled()) {
                sprintf(cmd, "usb_update %s %s.bin.signed",
                        partition[index], partition[index]);
            } else {
                sprintf(cmd, "usb_update %s %s.bin",
                        partition[index], partition[index]);
            }
            ret += run_command(cmd, 1);
        } else if (!strcmp(partition[index], "system")) {//update system
            sprintf(cmd, "store erase partition system");
            ret += run_command(cmd, 1);
            sprintf(cmd, "usb_update system system.img");
            ret += run_command(cmd, 1);
        } else if (!strcmp(partition[index], "userdata")) {//update userdata
            sprintf(cmd, "store erase partition data");
            ret += run_command(cmd, 1);
            sprintf(cmd, "usb_update data userdata.img");
            ret += run_command(cmd, 1);
        } else {//update others
            sprintf(cmd, "store erase partition %s", partition[index]);
            ret += run_command(cmd, 1);
            sprintf(cmd, "usb_update %s %s.img", partition[index], partition[index]);
            ret += run_command(cmd, 1);
        }

        show_flash_progress(index + 1, num, ret);
        if (ret)
            goto flash_image_error;
    }

    printf("Flash Image Finish\n");

    // FIXUP ONLY DEMO
    run_command("reboot", 1);
flash_done:
    while (1)
		mdelay(1000);
flash_image_error:
    printf("Flashing image %s error, update failed\n", partition[index]);
    goto flash_done;
flash_idme_error:
    printf("Flashing idme error\n");
    goto flash_done;

}

int do_uboot_update (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int ret;
    ulong addr = 0;
    int index = 0;
    char out[MAX_LINE_BUFF] = {0};

    if(amzn_target_is_lockdown()){
        printf("target is lockdonw , please unlock!\n");
        goto error;
    }

    #ifndef USB_UPGRADE_IN_ONE_FILE
    ret = update_ui_init();
    if (ret < 0) {
        printf("Image flashing GUI init failure\n");
        goto error;
    } else {
        show_flash_progress(0, 0, 0);
        printf("Image flashing GUI init done\n");
    }
    #endif

    init_param();
    idme_set();

    //get flash_script filesize
    int filesize = (int)getenv_hex("filesize", 0);

    //malloc buf for list
    char *buf = (char *)malloc(filesize+4);
    if (buf == NULL) {
        printf("malloc buffer(%d) failed!\n", filesize);
        goto error;
    }

    //get flash_script
    addr = simple_strtoul(argv[1], NULL, 16);
    memset(buf, 0, filesize+4);
    memcpy(buf, (char *)addr, filesize);

    buf[filesize]= '\0';
    printf("flash_script(%d):%s\n", filesize, buf );

    //parse flash_script
    char *temp = strtok(buf,"\n");
    while(temp)
    {
        memset(out, 0, MAX_LINE_BUFF);
        strtrim(temp, out);
        if (!idme_parse(out)) {
            temp = strtok(NULL,"\n");
            continue;
        }

        memset(partition[num], 0, 32);
        strcpy(partition[num], out);
        num++;
        temp = strtok(NULL,"\n");
    }

    //free
    free(buf);
    buf=NULL;

    if(arb_check_version() < 0)
	    goto error;

    //check image whether exist
    char image[32] = {0};
    for (index=0; index<num; index++) {
        printf("need to update %s partition, for check......\n", partition[index]);
        if (!strcmp(partition[index], "bootloader")) {
            printf("Attention: if it's secure SOC, need to update signed bootloader.\n");
            if (true == secure_boot_enabled()) {
                sprintf(image, "%s.bin.signed", partition[index]);
            } else {
                sprintf(image, "%s.bin", partition[index]);
            }
        } else {
            sprintf(image, "%s.img", partition[index]);
        }

        if (image_check(image) == 0) {
            #ifdef USB_UPGRADE_IN_ONE_FILE
            printf("%s is not exist, try usb_burn\n", image);
            ret += idme_set();
            //if (ret)
            //       goto error;
            printf("go to usb_burn\n");
            return 1;
            #else
            printf("%s is not exist, break......\n", image);
            goto error;
            #endif
        }
        printf("%s is exist\n", image);
    }

    #ifdef USB_UPGRADE_IN_ONE_FILE
    ret = update_ui_init();
    if (ret < 0) {
        printf("Image flashing GUI init failure\n");
           goto error;
    } else {
        show_flash_progress(0, 0, 0);
        printf("Image flashing GUI init done\n");
    }
    #endif

    image_update();

flash_secure_error:
error:
    show_flash_progress(1, 1, 1);
    while(1)
        mdelay(1000);
    return 0;
}
#else
int do_uboot_update (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
    return 0;
}

#endif

U_BOOT_CMD(
	uboot_update, 2, 0,	do_uboot_update,
	"run uboot update from usb device by flash_script",
	"[addr] - flash script starting at addr\n"
);


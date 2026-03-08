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
char partition[16][32];

typedef struct {
    char name[NAME_MAX];
    char value[VALUE_MAX];
} idme_t;

int nidme = 0;
idme_t idme_data[MAX_IDME_NUM];

static u32 fb_width;
static u32 fb_height;
static u32 display_bpp;
static unsigned char *fb_addr;
extern unsigned long get_fb_addr(void);

#define COLOR_RED (0xFF << 16)
#define COLOR_GREEN (0xFF << 8)
#define COLOR_BLUE (0xFF << 0)
#define COLOR_WHITE (COLOR_RED | COLOR_GREEN | COLOR_BLUE)

static int show_color_block(u32 color, int pos_x, int pos_y, int w, int h)
{
    int i, j;
    unsigned char *fbp = NULL;
    int byte_per_pixel;

    if (!fb_addr) {
        printf("Framebuffer wasn't initialized\n");
        goto error;
    }

    if (pos_x < 0 || w < 0 ||
            pos_y < 0 || h < 0 ||
            (pos_x + w) > fb_width ||
            (pos_y + h) > fb_height) {
        printf("Position is out of range, image upgrade failed\n");
        goto error;
    }

    byte_per_pixel = display_bpp / 8;
    fbp = fb_addr + (fb_width * pos_y + pos_x) * byte_per_pixel;

    for (i = 0; i < h ; i++) {
        for (j = 0; j < w; j++) {
            *(fbp + 0) = color & 0xFF;
            *(fbp + 1) = (color >> 8) & 0xFF;
            *(fbp + 2) = (color >> 16) & 0xFF;
            fbp += byte_per_pixel;
        }
        fbp += (fb_width - w) * byte_per_pixel;
    }

    flush_cache(fb_addr, fb_width * fb_height * byte_per_pixel);

    return 0;
error:
    return -1;
}

static int show_flash_progress(int cur_step, int total_steps, int result)
{
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
    color = !(cur_step | total_steps) ? COLOR_WHITE :
        result ? COLOR_RED : COLOR_GREEN;

    show_color_block(color, x, y, w, h);

    return 0;
error:
    return -1;
}

static int update_ui_init(void)
{
    char *str = NULL;

    fb_addr = (unsigned char *)get_fb_addr();

    str = getenv("fb_width");
    fb_width = str ? simple_strtoul(str, NULL, 10) : 0;

    str = getenv("fb_height");
    fb_height = str ? simple_strtoul(str, NULL, 10) : 0;

    str = getenv("display_bpp");
    display_bpp = str ? simple_strtoul(str, NULL, 10) : 0;
    if (display_bpp != 24) {
        printf("Only 24bpp is supported now\n");
        goto error;
    }

    if (!(fb_width && fb_width && display_bpp && fb_addr))
        goto error;

    return 0;
error:
    return -1;
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
    memcpy(buff, idme, MAX_LINE_BUFF);

    char *pb = strstr(buff, "=");
    if (!pb) {
        //not idme
        return -1;
    }

    int len = strlen(buff);
    int offset = pb - buff;
    if (nidme > MAX_IDME_NUM-1) {
        printf("max value(%d) support idme set!\n", MAX_IDME_NUM);
        return 0;
    }

    //get idme name and value
    memcpy(idme_data[nidme].name, buff, offset);
    memcpy(idme_data[nidme].value, buff+offset+2, len-offset-3);
    nidme++;
    return 0;
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

#if defined(CONFIG_BLANCHE)
extern bool secure_boot_enabled(void);
#else
static bool secure_boot_enabled(void)
{
    return false;
}
#endif

void image_update(void ) {
    int index = 0, ret = 0;
    char cmd[128] = {0};

    ret += idme_set();
    if (ret)
        goto flash_idme_error;

#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
    run_command("led_mode 4", 1);
#endif
    //update image
    for (index=0; index<num; index++) {
        if (!strcmp(partition[index], "dtb")) {//update mbr
            sprintf(cmd, "fatload usb 0 ${loadaddr} %s.img", partition[index]);
            ret += run_command(cmd, 1);
            sprintf(cmd, "store mbr ${loadaddr}");
            ret += run_command(cmd, 1);
        } else if (!strcmp(partition[index], "bootloader")) {//update bootloader
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
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
    run_command("led_mode 3", 1);
#endif

    printf("All images were upgraded successfull\n");

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

extern int is_locked_production_device(void);
extern int amzn_device_is_sec_flashing_enabled(void);
int do_uboot_update (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int ret;
    ulong addr = 0;
    int index = 0;
    char out[MAX_LINE_BUFF] = {0};

    ret = update_ui_init();
    if (ret < 0) {
        printf("Image flashing GUI init failure\n");
        goto error;
    } else {
        show_flash_progress(0, 0, 0);
        printf("Image flashing GUI init done\n");
    }

    if (is_locked_production_device()) {
        if (!amzn_device_is_sec_flashing_enabled()) {
            printf("Secure flash authorization failed, image upgrade error\n");
            goto flash_secure_error;
        }
    }

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
            printf("%s is not exist, break......\n", image);
            goto error;
        }
        printf("%s is exist\n", image);
    }

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
    uboot_update, 2, 0,    do_uboot_update,
    "run uboot update from usb device by flash_script",
    "[addr] - flash script starting at addr\n"
);


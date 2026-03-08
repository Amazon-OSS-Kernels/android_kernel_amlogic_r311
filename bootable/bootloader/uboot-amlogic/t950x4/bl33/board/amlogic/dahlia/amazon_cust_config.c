
/*
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <environment.h>
#include <fdt_support.h>
#include <libfdt.h>

#include <asm/cpu_id.h>
#include <asm/arch/secure_apb.h>
#include <asm-generic/gpio.h>
#include <linux/sizes.h>
#include <dm.h>
#include <asm/arch/io.h>
#include "amlogic/leds_state.h"

//add amzn start
#ifdef CONFIG_IDME
#include <idme.h>
/*amazon use idme 'mode_name' to specify model config file*/
#define MODEL_CONFIG_FILE_PATH "model_name"
#endif
/*amazon no idme 'config_name' to specify model name,just define a default*/
#define MODEL_NAME_DEFAULT "DEFAULT"
//add amzn end


/*
==========================Board info BEGIN
*/
#ifdef CONFIG_VENDOR_BOARDINFO
static struct  {
    int board_info;
    char * board_id;
} board_map[] = {
    {AMA_REF_BOARD_ID_TYPE, AMA_REF_BOARD_ID},  // Amazon reference square board
    {AMA_L4_BOARD_ID_TYPE , AMA_L4_BOARD_ID},   // Amazon reference triangle board - 4 layer
    {AMA_L2_BOARD_ID_TYPE , AMA_L2_BOARD_ID},   // Amazon reference triangle board - 2 layer
    {PROTO_BOARD_ID_TYPE,   PROTO_BOARD_ID},    // Amlogic Proto (depend on ODM)


    {REF_BOARD_ID_TYPE,     REF_BOARD_ID},      // Amlogic reference board
    {HVT_BOARD_ID_TYPE,     HVT_BOARD_ID},
    {EVT_BOARD_ID_TYPE,     EVT_BOARD_ID},
    {DVT_BOARD_ID_TYPE,     DVT_BOARD_ID},
    {PVT_BOARD_ID_TYPE,     PVT_BOARD_ID},
};

static int get_boardinfo(void)
{
    char buf[24] = {0};
    int i = 0;
    char tmp[10] = {0};
    if (idme_get_var_external("board_id", buf, sizeof(buf))) {
        printf("Can't get board id from IDME\n");
        return -1;
    }

    for (i=0; i< (sizeof(board_map)/sizeof(board_map[0])); i++) {
        if (0 == strncmp(buf, board_map[i].board_id, strlen(board_map[i].board_id))) {
            snprintf(tmp, sizeof(tmp), "%d", board_map[i].board_info);
            setenv("boardinfo", tmp);
            return board_map[i].board_info;
        }
    }
    printf("Don't support board_id %s\n", buf);
    return -1;
}
#endif
/*
==========================Board info END
*/

 /* the function will add string in bootargs. the name and value can't be set to NULL*/
 int append_bootargs(char * name, char * value)
 {
	 char *oldbootargs = NULL;
	 char *add_str	   = NULL;
	 char *newbootargs = NULL;
	 char *find_str    = NULL;
	 int ret = -1;

	 oldbootargs = getenv("bootargs");
	 if (oldbootargs == NULL)
	 {
		 printf("Can't get bootargs\n");
		 return ret;
	 }

	 //  the +2 inlcude "\0 and "="
	 add_str = malloc(strlen(name) + strlen(value) + 2);
	 if (add_str == NULL) {
		 printf("aml log : internal sys error!\n");
		 return ret;
	 }
	 snprintf(add_str, strlen(name) + strlen(value) + 2, "%s=%s", name, value);

	 //  the +2 inlcude "\0 and "="
	 find_str = malloc(strlen(name) + 2);
	 if (find_str == NULL) {
		 printf("aml log : internal sys error!\n");
		 goto free_mem;
	 }
	 snprintf(find_str, strlen(name) + 2, "%s=", name);

	 // the +2 inlcude "\0 and " "
	 newbootargs = malloc(strlen(oldbootargs) + strlen(add_str) + 2);
	 if (newbootargs == NULL)
	 {
		 printf("aml log : internal sys error!\n");
		 goto free_mem;
	 }

	 memset(newbootargs, 0, strlen(oldbootargs) + strlen(add_str) + 2);
	 char *pFind = strstr(oldbootargs, find_str);
	 if (pFind == NULL)
		 // Add new name=value
		 snprintf(newbootargs, strlen(oldbootargs) + strlen(add_str) + 2, "%s %s",oldbootargs, add_str);
	 else {
		 // Find if there is already name str in the old string
		 // Copy the string before pFind
		 memcpy(newbootargs, oldbootargs, pFind - oldbootargs);

		 // Add new str
		 memcpy(newbootargs + strlen(newbootargs), add_str, strlen(add_str));

		 // Add string after pfind
		 pFind = strstr(pFind, " ");
		 if (pFind != NULL) {
			 memcpy(newbootargs + strlen(newbootargs), pFind, strlen(pFind));
		 }
	 }

	 setenv("bootargs",newbootargs);
	 ret = 0;

 free_mem:
	 if (newbootargs != NULL) {
		 free(newbootargs);
		 newbootargs = NULL;
	 }

	 if (find_str != NULL) {
		 free(find_str);
		 find_str = NULL;
	 }

	 if (add_str != NULL) {
		 free(add_str);
		 add_str = NULL;
	 }
	 return ret;
 }

 /*
  *The function will return lcd backlight is on or off. 0->off, 1->on
  *AO_RTI_STATUS_REG0   bit28 is backlight control pin, so we get the pin's status high or low
  **********ONLY BE USED TO DAHLIA**********
 */
 int get_lcd_bl_status(void) {
    uint32_t val;

    val = readl(AO_RTI_STATUS_REG0);
    val = (val & (1 << 28)) >> 28;
    printf("get_lcd_bl_status:%s\n", val ? "on":"off");
    return val;
 }



#if defined(CONFIG_IDME)
static int board_id_type_check(void)
{
    char buf[24] = {0};
    int rtn = HVT_BOARD_ID_TYPE;

    if (!idme_get_var_external("board_id", buf, sizeof(buf))) {
        printf("board_id = %s\n", buf);
        if ((0 == strncmp(buf, HVT_BOARD_ID,strlen(HVT_BOARD_ID))) || 0 == strncmp(buf, HVT2_BOARD_ID,strlen(HVT2_BOARD_ID))){
            rtn = HVT_BOARD_ID_TYPE;
            setenv("hw_version", "HVT");
        }else if ((0 == strncmp(buf, EVT_BOARD_ID,strlen(EVT_BOARD_ID)))){
            rtn = EVT_BOARD_ID_TYPE;
            setenv("hw_version", "EVT");
        }else if ((0 == strncmp(buf, DVT_BOARD_ID,strlen(DVT_BOARD_ID)))){
            rtn = DVT_BOARD_ID_TYPE;
            setenv("hw_version", "DVT");
        }else if ((0 == strncmp(buf, PVT_BOARD_ID,strlen(PVT_BOARD_ID)))){
            rtn = PVT_BOARD_ID_TYPE;
            setenv("hw_version", "PVT");
        }else if ((0 == strncmp(buf, REF_BOARD_ID,strlen(REF_BOARD_ID)))){
            rtn = REF_BOARD_ID_TYPE;
            setenv("hw_version", "REF");
        }
    }
    return rtn;
}

unsigned long amz_dev_flags_check(void)
{
    char buf[24] = "";
    unsigned long rtn = 0;
    char tmp_buf[8] = "";

    if (!idme_get_var_external("dev_flags", buf, sizeof(buf))){
        printf("dev_flags = %s\n", buf);
        rtn = simple_strtoul (buf, NULL, 16);
    }
    if (rtn & DEV_FLAGS_BYPASS_SECONDARY_BOOT) {
        snprintf(tmp_buf, sizeof(tmp_buf), "%d", 1);
        setenv("bypass_standby", tmp_buf);
    }

    return rtn;
}


#define BUILD_TAG_LEN 128
#define BUILD_INFO_LEN 128
extern void get_build_tag() {
    char buf[BUILD_TAG_LEN] = "unknown";
#if defined BUILD_TAG
    char * info_start = strstr(BUILD_TAG,"build");
    if(info_start && ((strlen(CONFIG_DEVICE_PRODUCT) + strlen(info_start))<(BUILD_TAG_LEN-18))){
        memset(buf,0,BUILD_TAG_LEN);
        if (strlen(BUILD_TAG) < BUILD_INFO_LEN){
            snprintf(buf, sizeof(buf), "%s_Uboot_AMZN_%s", CONFIG_DEVICE_PRODUCT,info_start);
        }
    }
#else
    if(strlen(CONFIG_DEVICE_PRODUCT)<(BUILD_TAG_LEN-24)){
        memset(buf,0,BUILD_TAG_LEN);
        snprintf(buf, sizeof(buf), "%s_Uboot_%s", CONFIG_DEVICE_PRODUCT,"localbuild");
    }
#endif

#if defined UBOOT_BUILD_TAG_SUFFIX_DIRTY
    strcat(buf,"_DIRTY");
#endif
    printf("UbootBuildTag---------------%s\n",buf);
    setenv("UbootBuildTag",buf);
}

static bool store_demo_mode(void)
{
    char usr_flags_buf[8] = {0};
    unsigned usr_flags = 0;
    bool store_demo_mode = false;

    /* treat usr_flags as an unsigned integer in hex */
    if (!idme_get_var_external("usr_flags", usr_flags_buf, sizeof(usr_flags_buf) - 1)) {
        usr_flags = simple_strtoul(usr_flags_buf, NULL, 16);
        if (usr_flags & USR_FLAGS_STOREDEMO_MODE) {
            store_demo_mode = true;
#ifdef CONFIG_MESON_LEDS_STATE_CONTROL
            run_command("leds_state 0 2 4", 0);
#endif
            printf("cold boot directly,breathing\n");
        }
    }

    printf("store demo mode: %d\n", store_demo_mode);
    return store_demo_mode;
}
#endif


#ifdef CONFIG_HARDWARE_ID
char hwid[10]={0};
static void print_hardware_id()
{
    unsigned int hwid_tmp = 0;
    int i =0;
    char buf[10] = {0};
    //GPIOZ_0 ~ GPIOZ_3

    clrbits_le32(P_PERIPHS_PIN_MUX_4, 0xFFFF);//set pinmux --gpio mode
    setbits_le32(P_PREG_PAD_GPIO1_EN_N,(0xF));//set pinmux --gpio input
    hwid_tmp = (readl(P_PREG_PAD_GPIO1_I)) & 0xF;//get value
    for(i = 3; i >= 0; --i){
        snprintf(buf, sizeof(buf), "%d", ((hwid_tmp >> i) & 0x1));
        strcat(hwid, buf);
    }

    // THE PROTO BOARD IS 1010 (GPIOZ_3 ~ GPIOZ_0)
    printf("HW ID is %s\n",hwid);
    setenv("hwid",hwid);
}

static int get_saradc_val(unsigned int channel)
{
    saradc_enable();
    int val = get_adc_sample_gxbb(channel);
    saradc_disable();
    return val;
}


static void print_hardware_adc_id()
{
    int val = -1;
    // PROTO BOARD the hw adc channel is 0;
    val = get_saradc_val(0);
    printf("HW saradc is %d\n", val);
}

static void print_ssw_id()
{
    int val = -1;
    // PROTO BOARD the ssw adc channel is 2;
    val = get_saradc_val(2);
    printf("SSW saradc is %d\n", val);
}
#endif




extern int ft_board_setup(void *blob, bd_t *bd)
{
    struct fdt_header *fdt_ptr = (struct fdt_header *)blob;
    unsigned int newsize = fdt_totalsize(fdt_ptr) + CONFIG_IDME_SIZE;

    fdt_open_into(fdt_ptr, fdt_ptr, newsize);
    idme_device_tree_initialize(fdt_ptr);
    printf("IDME inserted into FDT\n");
    return 0;
}

static int get_logo_filepath(char *logo_path, int size)
{
    int ret = -1;
    char model_name[256] = "";
#if defined(CONFIG_IDME)
    if (idme_get_var_external("model_name", model_name, sizeof(model_name)) != 0) {
        printf("can not get model_name ! \n");
        return ret;
        }
    printf("get idme model_name: %s\n", model_name);
#else
    #error "Must defined CONFIG_IDME"
#endif
    const char *ini_value = NULL;
    IniParserInit();

    if (IniParseFile(model_name) < 0) {
        printf("%s, model ini load file error!\n", __func__);
        goto exit;
    }

    ini_value = IniGetString(MODEL_NAME_DEFAULT, "BOOTUP_LOGO_FILE_PATH", "null");
    if (strcmp(ini_value, "null") == 0) {
        printf("%s, get \"BOOTUP_LOGO_FILE_PATH\" item failed!\n", __func__);
        goto exit;
    }

    memset(logo_path, 0 , size);
    strncpy(logo_path, ini_value, size - 1);

    ret = 0;
exit:
    IniParserUninit();
    return ret;
}

static int do_logo_display(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
    char cmd_buf[512]     = { 0 };
    char logo_path[256]   = { 0 };
    int  file_size        = 0;
    unsigned int mem_addr = 0;

    if (argv[1] == NULL) {
        return 1;
    }

    if (get_logo_filepath(logo_path, sizeof(logo_path)) != 0 ) {
        printf("Can't get logo path\n");
        goto read_logo;
    }

    file_size = iniGetFileSize(logo_path);
    if (file_size <= 0) {
        printf("Can't get logo file size\n");
        goto read_logo;
    }
    char * loadaddr = getenv("loadaddr");
    if (loadaddr == NULL) {
        printf("Can't get logo memory addr\n");
        goto read_logo;
    }

    mem_addr = simple_strtoul(loadaddr, NULL, 16);
    if (iniReadFileToBuffer(logo_path, 0, file_size, mem_addr) <=0 ) {
        printf("Read logo file error\n");
        goto read_logo;
    }

    printf("Show logo from tvconfig\n");
    snprintf(cmd_buf, sizeof(cmd_buf), "bmp display $loadaddr");
    run_command(cmd_buf, 0);
    return 0;

read_logo:
    printf("Show logo from logo partition\n");
    snprintf(cmd_buf, sizeof(cmd_buf), "imgread pic logo %s $loadaddr", argv[1]);
    run_command(cmd_buf, 0);

    snprintf(cmd_buf, sizeof(cmd_buf),"bmp display $%s_offset", argv[1]);
    run_command(cmd_buf, 0);
    return 0;
}

U_BOOT_CMD(
    logo_display, 3, 0, do_logo_display,
    "logo_display",
    "logo_display\n"
);


extern int do_setMtkBT( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
U_BOOT_CMD(
    setMtkBT, CONFIG_SYS_MAXARGS, 1, do_setMtkBT,
    "load MTK BT driver, and set woble\n",
    NULL
);

/*
 *The function will set LED status,make sure rebootmode is ready to use
 */
static void update_led(void) {

	char *rebootmode = NULL;

	run_command("get_rebootmode", 0);

	rebootmode = getenv("reboot_mode");
	if (rebootmode == NULL) {
		printf("get rebootmode error! can't set LED\n");
		return ;
	}

	if(!strcmp(rebootmode,"factory_reset")){
		meson_ledstate_set_brightness(0,51);
		printf("reboot_mode = %s,set set 20 percent brightness\n",rebootmode);
	}

	if(!strcmp(rebootmode,"normal") || !strcmp(rebootmode,"shutdown_reboot")){
		meson_ledstate_set_breath(0,4);/* send date to bl30 */
		printf("reboot_mode = %s,breathing\n",rebootmode);
	}
	return ;
}


extern int amazon_hardware_init() {
    printf("****Amazon hardware init...\n");
#ifdef CONFIG_HARDWARE_ID
    print_hardware_id();
    print_hardware_adc_id();
    print_ssw_id();
#endif

    // FIXUP temp reset the 7668
    printf("power down wifi\n");
    run_command("gpio clear GPIOB_12",0);
    mdelay(200);
    run_command("gpio set GPIOB_12",0);
    mdelay(200);

    printf("reset wifi\n");
    run_command("gpio clear GPIOD_2",0);
    mdelay(200);
    run_command("gpio set GPIOD_2",0);
    mdelay(200);

#ifdef CONFIG_VENDOR_BOARDINFO
    get_boardinfo();
#endif
    return 0;
}

extern void idme_get_oem_data_field(const char *item, char *buf, unsigned buf_len);
static void amazon_ammo_config()
{
#define PROD_VAR_SIZE 32
    char ammo_pv[PROD_VAR_SIZE+1] = {0,};
    idme_get_oem_data_field("ammo_var=", ammo_pv, PROD_VAR_SIZE);
    setenv("ammo_pv",ammo_pv);
}
static int amazon_logo_config() {
#if defined(CONFIG_IDME)
#if 0
    printf("******disable amazon bootlogo for demo*******\n");
#else
     run_command("setenv logo_name amazonboot", 1);
#endif
#endif
    return 0;
}

extern int amazon_config_init() {

    update_led();

    printf("****Amazon config init...\n");
#if defined(CONFIG_IDME)
    char buf[256] = {0};
    if (store_demo_mode()) {
        setenv("bypass_standby", "1");
    }

    amazon_logo_config();
    printf("amz_dev_flags_check: 0x%lu\n", amz_dev_flags_check());
    amazon_ammo_config();
#endif
    return 0;
}

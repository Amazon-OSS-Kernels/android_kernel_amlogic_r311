/*
 * Author:  Shoufu Zhao <shoufu.zhao@amlogic.com>
 */

#include "ini_config.h"

#define LOG_TAG "model"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_proxy.h"
#include "ini_handler.h"
#include "ini_platform.h"
#include "ini_io.h"
#include "ini_proj_sum.h"
#include "model.h"
#include "errno.h"
//#define ITEM_DEBUG

#if (defined ITEM_DEBUG)
    #define ITEM_LOGD(x...) ALOGD(x)
    #define ITEM_LOGE(x...) ALOGE(x)
#else
    #define ITEM_LOGD(x...)
    #define ITEM_LOGE(x...)
#endif

#define CC_PARAM_CHECK_OK                             (0)
#define CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM        (-1)
#define CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM    (-2)

static int parse_panel_ini(const char *file_name, struct lcd_attr_s *lcd_attr, struct lcd_ext_attr_s *lcd_ext_attr, struct bl_attr_s *bl_attr, struct panel_misc_s *misc_attr);
static int check_param_valid(int mode, int parse_len, unsigned char parse_buf[], int ori_len, unsigned char ori_buf[]);

static int handle_integrity_flag(void);

static int handle_lcd_basic(struct lcd_attr_s* p_attr);
static int handle_lcd_timming(struct lcd_attr_s* p_attr);
static int handle_lcd_customer(struct lcd_attr_s* p_attr);
static int handle_lcd_interface(struct lcd_attr_s* p_attr);
static int handle_lcd_pwr(struct lcd_attr_s* p_attr);
static int handle_lcd_header(struct lcd_attr_s* p_attr);

static int handle_lcd_ext_basic(struct lcd_ext_attr_s* p_attr);
static int handle_lcd_ext_cmd_type(struct lcd_ext_attr_s* p_attr);
static int handle_lcd_ext_cmd_data(struct lcd_ext_attr_s* p_attr);
static int handle_lcd_ext_header(struct lcd_ext_attr_s* p_attr);

static int lcd_ext_cmd_data_to_buf(unsigned char tmp_buf[], struct lcd_ext_attr_s* p_attr);

static int handle_bl_basic(struct bl_attr_s* p_attr);
static int handle_bl_level(struct bl_attr_s* p_attr);
static int handle_bl_method(struct bl_attr_s* p_attr);
static int handle_bl_pwm(struct bl_attr_s* p_attr);
static int handle_bl_header(struct bl_attr_s* p_attr);

static int handle_panel_misc(struct panel_misc_s* p_misc);

#if (defined CC_COMPILE_IN_PC)
static int ExportDataAsDts(const char *fname_str, struct lcd_attr_s* p_lcd_attr, struct lcd_ext_attr_s *p_lcd_ext_attr, struct bl_attr_s* p_bl_attr);
static int ExportDataAsBin(const char *fname_str, void *data, int data_len);
static int export_panel_all_one_item(int index, unsigned char data_buf[]);
#endif

static int handle_panel_one_file_data(char *file_name, struct all_info_item_headers_s *head, unsigned char data_buf[]);

static int handle_hdr_attr(struct lcd_hdr_info_s* p_attr);

static int transBufferData(const char *data_str, unsigned int data_buf[]);

static int gLcdDataCnt = 0, gLcdExtDataCnt = 0, gBlDataCnt = 0;
static int g_lcd_pwr_on_seq_cnt = 0, g_lcd_pwr_off_seq_cnt = 0;

static int gLcdExtInitOnCnt = 0, gLcdExtInitOffCnt = 0;

#define CFG_FASTBOOT_MMC_NO    (1)
#define CONFIG_MMC_BLOCK_SIZE     512
#define CRI_RESERVE_NUM_OF_EMMC_BLOCKS   1024  /* reserve 512k for WB and Gamma; (512k)/512*/
#define CRI_CONFIG_NUM_OF_EMMC_BLOCKS    7168  /* reserve 512k for WB and Gamma; (4M -512k)/512*/


#define CRI_CONFIG_SIZE          (CRI_CONFIG_NUM_OF_EMMC_BLOCKS * CONFIG_MMC_BLOCK_SIZE)
#define CRI_CONFIG_OFFSET        (CRI_RESERVE_NUM_OF_EMMC_BLOCKS * CONFIG_MMC_BLOCK_SIZE)
#define CRI_MAGIC_NUMBER         "modelnam"

#define CRIDATA_MAX_NAME_LEN     64
struct cridata_desc {
	char name[CRIDATA_MAX_NAME_LEN];
	unsigned int size;
	unsigned int exportable;
	unsigned int permission;
};
struct cridata_item_t {
	struct cridata_desc desc;
	char data[0];
};
struct cridata_t {
  char magic[8];
  char version[4];
  unsigned int items_num;
  char item_data[0];
};

#ifndef MIN
#define MIN(x,y) ((x) > (y)? (y):(x))
#endif
  /* Align data in memory */
#define CRI_CONFIG_ALIGN_SIZE 4
#define CRI_CONFI_ITEM_NEXT(curr_item) \
	curr_item = (struct cridata_item_t *)((char *)curr_item + ((sizeof(struct cridata_desc) \
			+ curr_item->desc.size + CRI_CONFIG_ALIGN_SIZE - 1) & (~(CRI_CONFIG_ALIGN_SIZE - 1)))); \


static void setWpPin(void)
{
	int ret;
	unsigned int gpio;

	ret = gpio_lookup_name("GPIOZ_12", NULL, NULL, &gpio);
	if (ret) {
		printf("[%s, %d]lookup name GPIOZ_12 fail\n", __FUNCTION__, __LINE__);
		return;
	}

	ret = gpio_request(gpio, "WpPin");
	if (ret && ret != -EBUSY) {
		printf("[%s, %d] request GPIOZ_12 fail\n", __FUNCTION__, __LINE__);
		return;
	}
	gpio_direction_output(gpio, 0);
}

int cri_data_read_partion(char * model_name_dev,unsigned char *cri_buff ){
	char cmd[32] = {0};
	int ret = -1;
	struct cridata_t *pcri_data = NULL;
	char model_name_ini[CC_LCD_NAME_LEN_MAX]={0};

	if (!cri_buff) {
		ALOGE( "cri_buff cannot be NULL %s\n", __FUNCTION__);
		return ret;
	}
	sprintf(cmd, "amlmmc read cri_data  0x%llx  0x%llx  0x%llx ", cri_buff,CRI_CONFIG_OFFSET,CRI_CONFIG_SIZE);

	ret = run_command(cmd, 0);


	if (ret != 0 ) {
		ALOGE( "amlmmc read cri_data failed ! %s\n", __FUNCTION__);
		return ret;
	}
	pcri_data = (struct cridata_t *)&cri_buff[0];
	if (strncmp(pcri_data->magic, CRI_MAGIC_NUMBER, strlen(CRI_MAGIC_NUMBER))){
		ALOGE( "MAGIC_NUMBER is not matched to %s \n", CRI_MAGIC_NUMBER);
		ret = -1;
	}

	ret = cri_data_get_file(cri_buff ,model_name_ini , "model_name" , CC_LCD_NAME_LEN_MAX);
	if (ret){
		ALOGE( "Can not read data of item model_name\n");
		return ret;
	}else if (0 != strncmp(model_name_dev,model_name_ini,CC_LCD_NAME_LEN_MAX)){
		ALOGE( "model_name is not matched \n");
		ret = -1;
		return ret;
	}

	return ret;
}

int cri_data_get_file_length(unsigned char *cri_buff , char * name ){
	struct cridata_t *pcri_data = NULL;
	struct cridata_item_t *pitem = NULL;
	int i = 0;
	int length = -1;

	pcri_data = (struct cridata_t *)cri_buff;
	pitem = (struct cridata_item_t *)(&(pcri_data->item_data[0]));
	for (i = 0; i < pcri_data->items_num; i++) {

		if ( 0 == strcmp(name, pitem->desc.name) ) {
			length = pitem->desc.size;
			break;
		}else{
			CRI_CONFI_ITEM_NEXT(pitem);
		}
	}

	return length;
}

int cri_data_get_file(unsigned char *cri_buff ,unsigned char *pbuf , char * name , int length){
	struct cridata_t *pcri_data = NULL;
	struct cridata_item_t *pitem = NULL;
	int ret = -1;
	int i = 0;

	pcri_data = (struct cridata_t *)cri_buff;
	pitem = (struct cridata_item_t *)(&(pcri_data->item_data[0]));
	for (i = 0; i < pcri_data->items_num; i++) {
		if ( 0 == strcmp(name, pitem->desc.name) ) {
			memcpy( pbuf, &(pitem->data[0]), MIN( pitem->desc.size, length ) );
			ret = 0;
			break;
		}else{
			CRI_CONFI_ITEM_NEXT(pitem);
		}
	}
	return ret;
}



extern INI_HANDLER_DATA *gHandlerData ;
int parase_panel_file_from_cri_data(char * file_name){
	unsigned char *handle_buf = NULL;
	int length = -1;
	int ret = -1;
	unsigned char * cri_buff = NULL;
	char model_name_dev[CC_LCD_NAME_LEN_MAX] = {0};

	if (gHandlerData == NULL) {
		ALOGE( "gHandlerData cannot be NULL , shuold be init %s\n", __FUNCTION__);
		return ret;
	}

	if (idme_get_var_external("model_name",model_name_dev,CC_LCD_NAME_LEN_MAX)){
		ALOGE( "can not get model name %s\n", __FUNCTION__);
		return ret;
	}

	cri_buff = (unsigned char *) malloc(CRI_CONFIG_SIZE);
	if(cri_buff == NULL) {
		ALOGE( "cannot alloc memory for cri_buff  %s\n", __FUNCTION__);
		return ret;
	}
	memset(cri_buff,0,CRI_CONFIG_SIZE);

	if (cri_data_read_partion(model_name_dev,cri_buff)){
		ALOGE( "Can not read cri_data partition %s\n", __FUNCTION__);
		free(cri_buff);
		return ret;
	}

	length = cri_data_get_file_length(cri_buff , file_name );
	if (!(length > 0)){
		ALOGE( "read length of file , it is not right ! %s\n", __FUNCTION__);
		free(cri_buff);
		return ret;
	}

	handle_buf = (unsigned char *) malloc(length * 2);

	if (handle_buf == NULL) {
		ALOGE( "Can not alloc memory for handle_buf %s\n", __FUNCTION__);
		free(cri_buff);
		return ret;
	}
	memset(handle_buf,0,length * 2);

	ret = cri_data_get_file(cri_buff ,handle_buf , file_name , length);
	if (ret){
		ALOGE( "Can not read data of file %s\n", __FUNCTION__);
		free(cri_buff);
		free(handle_buf);
		return ret;
	}

	strncpy(gHandlerData->mpFileName, file_name, CC_MAX_INI_FILE_NAME_LEN - 1);
        ret = ini_mem_parse(handle_buf+4, gHandlerData);
	free(cri_buff);
	free(handle_buf);
        handle_buf = NULL;
	return ret;
}


int handle_panel_ini(const char *file_name) {
    int tmp_len = 0;
    unsigned char *tmp_buf = NULL;
    unsigned char *parse_buf = NULL;
    struct lcd_attr_s lcd_attr;
    struct lcd_ext_attr_s lcd_ext_attr;
    struct bl_attr_s bl_attr;
    struct panel_misc_s misc_attr;
    char part_name[CC_MAX_INI_FILE_NAME_LEN]={0};
    char subfile_name[CC_MAX_INI_FILE_NAME_LEN]={0};

    tmp_buf = (unsigned char *) malloc(CC_MAX_DATA_SIZE);
    if (tmp_buf == NULL) {
        ALOGE("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    parse_buf = (unsigned char *) malloc(CC_MAX_DATA_SIZE);
    if (parse_buf == NULL) {
        free(tmp_buf);
        tmp_buf = NULL;
        ALOGE("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    memset((void *)&lcd_attr, 0, sizeof(struct lcd_attr_s));
    memset((void *)&lcd_ext_attr, 0, sizeof(struct lcd_ext_attr_s));
    memset((void *)&bl_attr, 0, sizeof(struct bl_attr_s));
    memset((void *)&misc_attr, 0, sizeof(struct panel_misc_s));

    //init misc attr as default
    strcpy(misc_attr.version, "V001");
    strcpy(misc_attr.reverse, "no_rev");
    strcpy(misc_attr.outputmode, "2160p60hz");


    if ((splitFilePath(file_name, part_name, subfile_name, NULL) < 0)) {
         return -1;
    }

    if(strncmp(part_name,"cri_data",strlen("cri_data"))){
	if (!iniIsFileExist(file_name)) { // start handle panel ini name
		ALOGE("%s, file name not exist.\n", __FUNCTION__);
		free(tmp_buf);
		tmp_buf = NULL;
		free(parse_buf);
		parse_buf = NULL;
		return -1;
	}
    }else{
	printf("%s, cri logic for file  \n", __FUNCTION__);
    }

   if (parse_panel_ini(file_name, &lcd_attr, &lcd_ext_attr, &bl_attr, &misc_attr) < 0) {
        free(tmp_buf);
        tmp_buf = NULL;
        free(parse_buf);
        parse_buf = NULL;
        return -1;
    }

    // start handle lcd param
    memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
    tmp_len = ReadLCDParam(tmp_buf);
    //ALOGD("%s, start check lcd param data (0x%x).\n", __FUNCTION__, tmp_len);
    if (check_param_valid(0, gLcdDataCnt, (unsigned char*)&lcd_attr, tmp_len, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
        ALOGD("%s, check lcd param data error (0x%x), save lcd param.\n", __FUNCTION__, tmp_len);
        SaveLCDParam(gLcdDataCnt, (unsigned char*)&lcd_attr);
    }
    // end handle lcd param

    // start handle lcd extern param
    memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
    tmp_len = ReadLCDExternParam(tmp_buf);

    if (lcd_ext_attr.basic.ext_status == 0) {
        memset((void *)&lcd_ext_attr, 0, sizeof(struct lcd_ext_attr_s));
        lcd_ext_attr.head.data_len = gLcdExtDataCnt;
        lcd_ext_attr.head.crc32 = CalCRC32(0, (((unsigned char *)&lcd_ext_attr) + 4), gLcdExtDataCnt - 4);
    }

    memset((void *)parse_buf, 0, CC_MAX_DATA_SIZE);
    lcd_ext_cmd_data_to_buf(parse_buf, &lcd_ext_attr);

    //ALOGD("%s, start check lcd extern param data (0x%x).\n", __FUNCTION__, tmp_len);
    if (check_param_valid(0, gLcdExtDataCnt, parse_buf, tmp_len, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
        ALOGD("%s, check lcd extern param data error (0x%x), save lcd extern param.\n", __FUNCTION__, tmp_len);
        SaveLCDExternParam(gLcdExtDataCnt, parse_buf);
    }
    // end handle lcd extern param

    // start handle backlight param
    memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
    tmp_len = ReadBackLightParam(tmp_buf);
    //ALOGD("%s, start check backlight param data (0x%x).\n", __FUNCTION__, tmp_len);
    if (check_param_valid(0, gBlDataCnt, (unsigned char*)&bl_attr, tmp_len, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
        ALOGD("%s, check backlight param data error (0x%x), save backlight param.\n", __FUNCTION__, tmp_len);
        SaveBackLightParam(gBlDataCnt, (unsigned char*)&bl_attr);
    }
    // end handle backlight param

    // start handle panel misc
    memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
    strcpy((char *)tmp_buf, misc_attr.version);
    strcat((char *)tmp_buf, ",");
    strcat((char *)tmp_buf, misc_attr.reverse);
    strcat((char *)tmp_buf, ",");
    strcat((char *)tmp_buf, misc_attr.outputmode);
    unsigned int tmp_crc32 = CalCRC32(0, tmp_buf, strlen((char *)tmp_buf));
    sprintf((char *)parse_buf, "%08x,%s", tmp_crc32, (char *)tmp_buf);

    memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
    tmp_len = ReadPanelMiscInfo((char *)tmp_buf);
    //ALOGD("%s, start check panel misc data (0x%x).\n", __FUNCTION__, tmp_len);
    if (check_param_valid(1, 0, parse_buf, 0, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
        ALOGD("%s, check panel misc info error (0x%x), save panel misc info.\n", __FUNCTION__, tmp_len);
        strcpy((char *)tmp_buf, misc_attr.version);
        strcat((char *)tmp_buf, ",");
        strcat((char *)tmp_buf, misc_attr.reverse);
        strcat((char *)tmp_buf, ",");
        strcat((char *)tmp_buf, misc_attr.outputmode);
        SavePanelMiscInfo((char *)tmp_buf);
    }
    // end handle panel misc

#if (defined CC_COMPILE_IN_PC)
    ExportDataAsDts("panel.dts", &lcd_attr, &lcd_ext_attr, &bl_attr);
#endif

    free(tmp_buf);
    tmp_buf = NULL;
    free(parse_buf);
    parse_buf = NULL;

    return 0;
}

int handle_panel_pq_db_path(const char *file_name) {
    int tmp_len = 0, tmp_flag = 0;
    char tmp_path[256] = {0};

    // start handle panel pq db path
    if (file_name != NULL) {
        tmp_flag = 0;

        //ALOGD("%s, start check special panel pq db name \"%s\".\n", __FUNCTION__, file_name);

        if (!iniIsFileExist(file_name)) {
            ALOGE("%s, file name \"%s\" not exist.\n", __FUNCTION__, file_name);
            return -1;
        }

        tmp_len = ReadPanelPQPath(tmp_path);
        if (tmp_len <= 0 || strcmp(tmp_path, file_name)) {
            tmp_flag = 1;
        }

        if (tmp_flag == 1) {
            ALOGD("%s, save file as customer special name \"%s\"\n", __FUNCTION__, file_name);
            SavePanelPQPath((char *)file_name);
            return 1;
        }
    } else {
        tmp_flag = 0;

        tmp_len = ReadPanelPQPath(tmp_path);
        ALOGD("%s, start check panel pq db name \"%s\".\n", __FUNCTION__, tmp_path);
        if (tmp_len <= 0 || !iniIsFileExist(tmp_path)) {
            tmp_flag = 1;
        }

        if (tmp_flag == 1) {
            ALOGD("%s, save file as default name \"%s\"\n", __FUNCTION__, get_def_panel_pq_db_name());
            SavePanelPQPath((char *)get_def_panel_pq_db_name());
            return 1;
        }
    }
    // end handle panel pq db path

    return 0;
}
extern int splitFilePath(const char *file_path, char part_name[], char file_name[], const char *ext_name);
static int parse_panel_ini(const char *file_name, struct lcd_attr_s *lcd_attr, struct lcd_ext_attr_s *lcd_ext_attr, struct bl_attr_s *bl_attr, struct panel_misc_s *misc_attr) {
    memset((void *)lcd_attr, 0, sizeof(struct lcd_attr_s));
    memset((void *)bl_attr, 0, sizeof(struct bl_attr_s));

    IniParserInit();
    char part_name[CC_MAX_INI_FILE_NAME_LEN]={0};
    char subfile_name[CC_MAX_INI_FILE_NAME_LEN]={0};
    if ((splitFilePath(file_name, part_name, subfile_name, NULL) < 0)) {
	IniParserUninit();
         return -1;
    }
    strncpy(subfile_name,subfile_name+1,strlen(subfile_name));

    if(!strncmp(part_name,"cri_data",strlen("cri_data"))){

	if (parase_panel_file_from_cri_data(subfile_name) < 0){
		ALOGE("ini load file error!\n");
		IniParserUninit();
		return -1;
	}

    }else  if (IniParseFile(file_name) < 0) {
        ALOGE("%s, Parse ini  file error!\n", __FUNCTION__);
        IniParserUninit();
        return -1;
    }
    // handle integrity flag

    if (handle_integrity_flag() < 0) {
       IniParserUninit();
       return -1;
    }
    // handle lcd attr
    handle_lcd_basic(lcd_attr);
    handle_lcd_timming(lcd_attr);
    handle_lcd_customer(lcd_attr);
    handle_lcd_interface(lcd_attr);
    handle_lcd_pwr(lcd_attr);
    handle_lcd_header(lcd_attr);

    // handle lcd extern attr
    handle_lcd_ext_basic(lcd_ext_attr);
    handle_lcd_ext_cmd_type(lcd_ext_attr);
    handle_lcd_ext_cmd_data(lcd_ext_attr);
    handle_lcd_ext_header(lcd_ext_attr);

    // handle bl attr
    handle_bl_basic(bl_attr);
    handle_bl_level(bl_attr);
    handle_bl_method(bl_attr);
    handle_bl_pwm(bl_attr);
    handle_bl_header(bl_attr);

    handle_panel_misc(misc_attr);

    IniParserUninit();

    return 0;
}

int handle_save_one_sec_data(int sec_no, int wr_size, unsigned char data_buf[]) {
    int tmp_len = 0;
    unsigned char *pSecAllDataPtr = NULL;
    unsigned char *pSecAllReadDataPtr = NULL;
    struct all_one_sec_header_s *pHeadPtr = NULL;

    if (wr_size + sizeof(struct all_one_sec_header_s) > CC_ONE_SECTION_SIZE) {
        return -1;
    }

    pSecAllDataPtr = (unsigned char *) malloc(CC_ONE_SECTION_SIZE);
    if (pSecAllDataPtr == NULL) {
        ALOGE("%s, malloc one sec data buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    pSecAllReadDataPtr = (unsigned char *) malloc(CC_ONE_SECTION_SIZE);
    if (pSecAllReadDataPtr == NULL) {
        free(pSecAllDataPtr);
        pSecAllDataPtr = NULL;

        ALOGE("%s, malloc all panel read data buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    memset((void *)pSecAllDataPtr, 0, CC_ONE_SECTION_SIZE);
    memcpy((void *)(pSecAllDataPtr + sizeof(struct all_one_sec_header_s)), (void *)data_buf, wr_size);

    pHeadPtr = (struct all_one_sec_header_s *)pSecAllDataPtr;
    pHeadPtr->data_len = wr_size + sizeof(struct all_one_sec_header_s);
    pHeadPtr->version = 1;
    strncpy((char *)pHeadPtr->tag, CC_MAX_PANEL_ALL_ONE_SEC_TAG_CONTENT, CC_MAX_PANEL_ALL_ONE_SEC_TAG_SIZE);
    pHeadPtr->crc32 = CalCRC32(0, pSecAllDataPtr + 4, pHeadPtr->data_len - 4);

    memset((void *)pSecAllReadDataPtr, 0, CC_ONE_SECTION_SIZE);
    tmp_len = ReadPanelAllData(sec_no, pSecAllReadDataPtr);
    ALOGD("%s, start check panel all section data(%d, 0x%x).\n", __FUNCTION__, sec_no, pHeadPtr->data_len);
    if (check_param_valid(0, pHeadPtr->data_len, pSecAllDataPtr, tmp_len, pSecAllReadDataPtr) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
        ALOGD("%s, check panel all section data(%d, 0x%x), save the section data.\n", __FUNCTION__, sec_no, pHeadPtr->data_len);
        SavePanelAllData(sec_no, pHeadPtr->data_len, pSecAllDataPtr);
    }

    free(pSecAllReadDataPtr);
    pSecAllReadDataPtr = NULL;

    free(pSecAllDataPtr);
    pSecAllDataPtr = NULL;

    return 0;
}

int get_panel_ini_path(char *file_name, char * panel_name, struct project_info_s * pInfoPtr) {
    const char *ini_value = NULL;

    IniParserInit();

    if (IniParseFile(file_name) < 0) {
        ALOGE("%s, ini load file error!\n", __FUNCTION__);
        IniParserUninit();
        return -1;
    }

    ini_value = IniGetString(panel_name, "PANELINI_PATH", "null");
    if (strcmp(ini_value, "null") != 0) {
        strcpy(pInfoPtr->panel_ini_path, ini_value);
    }else{
	 printf(" connot find ini file  !\n");
    }

    ini_value = IniGetString(panel_name, "PQINI_PATH", "null");
    if (strcmp(ini_value, "null") != 0) {
        strcpy(pInfoPtr->panel_pq_path, ini_value);
    }

    ini_value = IniGetString(panel_name, "HDRINI_PATH", "null");
    if (strcmp(ini_value, "null") != 0) {
        strcpy(pInfoPtr->hdr_ini_path, ini_value);
    }

    ini_value = IniGetString(panel_name, "AUDIO_EQ_DRC_PATH", "null");
    if (strcmp(ini_value, "null") != 0) {
        strcpy(pInfoPtr->aud_eq_drc_ini_path, ini_value);
    }

    ini_value = IniGetString(panel_name, "AUDIO_CURVE_PATH", "null");
    if (strcmp(ini_value, "null") != 0) {
        strcpy(pInfoPtr->aud_curve_ini_path, ini_value);
    }

    IniParserUninit();

    return 0;
}

int handle_panel_ini_by_name(char *panel_name, char *file_name) {
    struct project_info_s panel_info;
    int ret = -1;
    char part_name[CC_MAX_INI_FILE_NAME_LEN]={0};
    char subfile_name[CC_MAX_INI_FILE_NAME_LEN]={0};

    memset(&panel_info, 0 , sizeof(struct project_info_s));
    if ((splitFilePath(file_name, part_name, subfile_name, NULL) < 0)) {
	 printf(" splitFilePath  does not exsit %s\n", __FUNCTION__);
         return ret;
    }

     if(!strncmp(part_name,"tvconfig",strlen("tvconfig"))){
	ret=get_panel_ini_path(file_name, panel_name, &panel_info);
	if ((ret == -1) || (strlen(panel_info.panel_ini_path) == 0 )){
		printf("can not find path of file %s !\n", __FUNCTION__);
		return -1;
	}
	handle_panel_ini(panel_info.panel_ini_path);
    }else{
	strcpy(panel_info.panel_ini_path, file_name);
	handle_panel_ini(panel_info.panel_ini_path);
   }
	/*remove PQ firstly since not load in uboot now*/
    /*handle_panel_pq_db_path(panel_info.panel_pq_path);*/
    return 0;
}

int handle_panel_ini_by_id(int panel_id, char *file_name) {
    int panel_cfg_cnt = 0, default_flag = 0;
    struct project_info_s *pInfoPtr = NULL;

    pInfoPtr = (struct project_info_s *) malloc(CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct project_info_s));
    if (pInfoPtr == NULL) {
        ALOGE("%s, malloc panel info memory error!!!\n", __FUNCTION__);
        return -1;
    }

    //ALOGD("%s, panel_id = %d, pane_info_name is (%s)\n", __FUNCTION__, panel_id, file_name);

    default_flag = CC_PANEL_INI_PATH_MASK | CC_PANEL_PQ_PATH_MASK;
    panel_cfg_cnt = handle_get_project_all_info(file_name, &default_flag, pInfoPtr);
    //ALOGD("%s, panel_cfg_cnt = %d, default flag (0x%08x)\n", __FUNCTION__, panel_cfg_cnt, default_flag);
    if ((default_flag & CC_PANEL_INI_PATH_MASK) && (default_flag & CC_PANEL_PQ_PATH_MASK)) {
        panel_cfg_cnt += 1;
    }

    if (panel_cfg_cnt > 0) {
        //refresh current panel id's ini and pq db path
        if (panel_id >= 0 && panel_id < panel_cfg_cnt - 1) {
            //ALOGD("%s, start refresh current panel id(%d)'s ini and pq db path.\n", __FUNCTION__, panel_id);

            handle_panel_ini(pInfoPtr[panel_id].panel_ini_path);
            handle_panel_pq_db_path(pInfoPtr[panel_id].panel_pq_path);
        } else {
            ALOGD("%s, use default ini and pq db path id(%d).\n", __FUNCTION__, panel_cfg_cnt - 1);

            if (default_flag & CC_PANEL_INI_PATH_MASK) {
                handle_panel_ini(pInfoPtr[panel_cfg_cnt - 1].panel_ini_path);
            } else {
                free(pInfoPtr);
                pInfoPtr = NULL;

                ALOGE("%s, there is no default panel ini path.\n", __FUNCTION__);
                return -1;
            }

            if (default_flag & CC_PANEL_PQ_PATH_MASK) {
                handle_panel_pq_db_path(pInfoPtr[panel_cfg_cnt - 1].panel_pq_path);
            } else {
                free(pInfoPtr);
                pInfoPtr = NULL;

                ALOGE("%s, there is no default panel pq path.\n", __FUNCTION__);
                return -1;
            }
        }
    } else {
        free(pInfoPtr);
        pInfoPtr = NULL;

        ALOGE("%s, there is no panel ini path and pq path.\n", __FUNCTION__);
        return -1;
    }

    free(pInfoPtr);
    pInfoPtr = NULL;

    return 0;
}

int handle_refresh_panel_info(int mode, int panel_id, char *file_name) {
    int i = 0, tmp_len = 0, tmp_val = 0, tmp_ind = 0, panel_cfg_cnt = 0, default_flag = 0;
    int tmp_off = 0, sec_cnt = 0;
    struct all_info_header_s *pHeadPtr = NULL;
    struct sec_item_s *pSecHeadPtr = NULL;
    struct all_info_item_headers_s *pItemHeadPtr = NULL;
    struct project_info_s *pInfoPtr = NULL;
    unsigned char *tmpAllInfoHeaderPtr = NULL;
    unsigned char *tmpAllInfoDataPtr = NULL;
    unsigned char *tmpAllInfoHeaderReadPtr = NULL;
    struct all_info_item_headers_s tmphead;

    pInfoPtr = (struct project_info_s *) malloc(CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct project_info_s));
    if (pInfoPtr == NULL) {
        ALOGE("%s, malloc panel info memory error!!!\n", __FUNCTION__);
        return -1;
    }

    ALOGD("%s, panel_id = %d, pane_info_name is (%s)\n", __FUNCTION__, panel_id, file_name);

    default_flag = CC_PANEL_INI_PATH_MASK | CC_PANEL_PQ_PATH_MASK;
    panel_cfg_cnt = handle_get_project_all_info(file_name, &default_flag, pInfoPtr);
    ALOGD("%s, panel_cfg_cnt = %d, default flag (0x%08x)\n", __FUNCTION__, panel_cfg_cnt, default_flag);
    if ((default_flag & CC_PANEL_INI_PATH_MASK) && (default_flag & CC_PANEL_PQ_PATH_MASK)) {
        panel_cfg_cnt += 1;
    }

    if (panel_cfg_cnt > 0) {
        if (mode == 0) {
            ALOGD("%s, mode is (%d), need to refresh current panel ini and pq db path.\n", __FUNCTION__, mode);
            //refresh current panel id's ini and pq db path
            if (panel_id >= 0 && panel_id < panel_cfg_cnt - 1) {
                ALOGD("%s, start refresh current panel id(%d)'s ini and pq db path.\n", __FUNCTION__, panel_id);

                handle_panel_ini(pInfoPtr[panel_id].panel_ini_path);
                handle_panel_pq_db_path(pInfoPtr[panel_id].panel_pq_path);
            } else {
                ALOGD("%s, use default ini and pq db path id(%d).\n", __FUNCTION__, panel_cfg_cnt - 1);

                if (default_flag & CC_PANEL_INI_PATH_MASK) {
                    handle_panel_ini(pInfoPtr[panel_cfg_cnt - 1].panel_ini_path);
                } else {
                    free(pInfoPtr);
                    pInfoPtr = NULL;

                    ALOGE("%s, there is no default panel ini path.\n", __FUNCTION__);
                    return -1;
                }

                if (default_flag & CC_PANEL_PQ_PATH_MASK) {
                    handle_panel_pq_db_path(pInfoPtr[panel_cfg_cnt - 1].panel_pq_path);
                } else {
                    free(pInfoPtr);
                    pInfoPtr = NULL;

                    ALOGE("%s, there is no default panel pq path.\n", __FUNCTION__);
                    return -1;
                }
            }
        } else {
            ALOGD("%s, mode is (%d), not need to refresh current panel ini and pq db path.\n", __FUNCTION__, mode);
        }
    } else {
        free(pInfoPtr);
        pInfoPtr = NULL;

        ALOGE("%s, there is no panel ini path and pq path.\n", __FUNCTION__);
        return -1;
    }

    // handle all panel data to key/bin
    tmpAllInfoHeaderPtr = (unsigned char *) malloc(CC_MAX_PANEL_ALL_DATA_SIZE);
    if (tmpAllInfoHeaderPtr == NULL) {
        free(pInfoPtr);
        pInfoPtr = NULL;

        ALOGE("%s, malloc all panel data buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    tmpAllInfoHeaderReadPtr = (unsigned char *) malloc(CC_MAX_PANEL_ALL_DATA_SIZE);
    if (tmpAllInfoHeaderReadPtr == NULL) {
        free(pInfoPtr);
        pInfoPtr = NULL;

        free(tmpAllInfoHeaderPtr);
        tmpAllInfoHeaderPtr = NULL;

        ALOGE("%s, malloc all panel data buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    tmpAllInfoDataPtr = (unsigned char *) malloc(CC_MAX_PANEL_ALL_DATA_SIZE);
    if (tmpAllInfoDataPtr == NULL) {
        free(pInfoPtr);
        pInfoPtr = NULL;

        free(tmpAllInfoHeaderPtr);
        tmpAllInfoHeaderPtr = NULL;

        free(tmpAllInfoHeaderReadPtr);
        tmpAllInfoHeaderReadPtr = NULL;

        ALOGE("%s, malloc all panel data buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    memset((void *)tmpAllInfoHeaderPtr, 0, CC_MAX_PANEL_ALL_DATA_SIZE);
    memset((void *)tmpAllInfoDataPtr, 0, CC_MAX_PANEL_ALL_DATA_SIZE);

    tmp_off = sizeof(struct all_info_header_s);
    pSecHeadPtr = (struct sec_item_s *)(tmpAllInfoHeaderPtr + tmp_off);

    tmp_off = sizeof(struct all_info_header_s) + CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct sec_item_s);
    pItemHeadPtr = (struct all_info_item_headers_s *)(tmpAllInfoHeaderPtr + tmp_off);

    sec_cnt = 0;
    tmp_off = 0;
    tmp_ind = 0;
    for(i = 0; i < panel_cfg_cnt; i++) {
        memset((void *)&tmphead, 0, sizeof(struct all_info_item_headers_s));

        tmp_len = handle_panel_one_file_data(pInfoPtr[i].panel_ini_path, &pItemHeadPtr[i], tmpAllInfoDataPtr + tmp_off);
        tmp_val = tmp_off + tmp_len + sizeof(struct all_one_sec_header_s);
        if (tmp_val == CC_ONE_SECTION_SIZE) {
            pItemHeadPtr[i].lcd.off += tmp_off;
            pItemHeadPtr[i].lcd_ext.off += tmp_off;
            pItemHeadPtr[i].backlight.off += tmp_off;
            pItemHeadPtr[i].panel_misc.off += tmp_off;
            pItemHeadPtr[i].rev1.off += tmp_off;
            pItemHeadPtr[i].rev2.off += tmp_off;

            // save panel's data to the current section, except the last panel
            handle_save_one_sec_data(sec_cnt, tmp_off + tmp_len, tmpAllInfoDataPtr);
            pSecHeadPtr[sec_cnt].start = tmp_ind;
            pSecHeadPtr[sec_cnt].end = i;
            sec_cnt += 1;

            tmp_ind = i + 1;

            tmp_off = 0;
            continue;
        } else if (tmp_val > CC_ONE_SECTION_SIZE) {
            // save panel's data to the current section, except the last panel
            handle_save_one_sec_data(sec_cnt, tmp_off, tmpAllInfoDataPtr);
            pSecHeadPtr[sec_cnt].start = tmp_ind;
            pSecHeadPtr[sec_cnt].end = i - 1;
            sec_cnt += 1;

            tmp_ind = i;

            // the last panel is not save in the current section.
            // move it's data to head and prepare to save in the next section.
            memcpy(tmpAllInfoDataPtr, tmpAllInfoDataPtr + tmp_off, tmp_len);

            tmp_off = 0;
            pItemHeadPtr[0].lcd.off += tmp_off;
            pItemHeadPtr[0].lcd_ext.off += tmp_off;
            pItemHeadPtr[0].backlight.off += tmp_off;
            pItemHeadPtr[0].panel_misc.off += tmp_off;
            pItemHeadPtr[0].rev1.off += tmp_off;
            pItemHeadPtr[0].rev2.off += tmp_off;

            tmp_off += tmp_len;
            continue;
        }

        pItemHeadPtr[i].lcd.off += tmp_off;
        pItemHeadPtr[i].lcd_ext.off += tmp_off;
        pItemHeadPtr[i].backlight.off += tmp_off;
        pItemHeadPtr[i].panel_misc.off += tmp_off;
        pItemHeadPtr[i].rev1.off += tmp_off;
        pItemHeadPtr[i].rev2.off += tmp_off;

        if (i + 1 == panel_cfg_cnt) {
            pSecHeadPtr[sec_cnt].start = tmp_ind;
            pSecHeadPtr[sec_cnt].end = i;

            handle_save_one_sec_data(sec_cnt, tmp_off + tmp_len, tmpAllInfoDataPtr);
        }

        tmp_off += tmp_len;
    }

    pHeadPtr = (struct all_info_header_s *)tmpAllInfoHeaderPtr;
    pHeadPtr->data_len = sizeof(struct all_info_header_s) + CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct sec_item_s) + CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct all_info_item_headers_s);
    pHeadPtr->version = 1;
    strncpy((char *)pHeadPtr->tag, CS_PANEL_ALL_INFO_TAG_CONTENT, CC_MAX_PANEL_ALL_INFO_TAG_SIZE);
    pHeadPtr->max_panel_cnt = CC_MAX_SUPPORT_PANEL_CNT;
    pHeadPtr->cur_panel_cnt = panel_cfg_cnt;
    pHeadPtr->sec_off = sizeof(struct all_info_header_s);
    pHeadPtr->sec_cnt = sec_cnt + 1;
    pHeadPtr->sec_size = CC_ONE_SECTION_SIZE;
    pHeadPtr->sec_len = CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct sec_item_s);

    pHeadPtr->item_head_off = sizeof(struct all_info_header_s) + CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct sec_item_s);
    pHeadPtr->item_head_cnt = panel_cfg_cnt;
    pHeadPtr->item_head_size = sizeof(struct all_info_item_headers_s);
    pHeadPtr->item_head_len = CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct all_info_item_headers_s);

    pHeadPtr->def_flag = default_flag;

    pHeadPtr->crc32 = CalCRC32(0, (((unsigned char *)tmpAllInfoHeaderPtr) + 4), pHeadPtr->data_len - 4);

    memset((void *)tmpAllInfoHeaderReadPtr, 0, CC_MAX_PANEL_ALL_DATA_SIZE);
    tmp_len = ReadPanelAllInfoData(tmpAllInfoHeaderReadPtr);
    ALOGD("%s, start check panel all info data(0x%x).\n", __FUNCTION__, pHeadPtr->data_len);
    if (check_param_valid(0, pHeadPtr->data_len, tmpAllInfoHeaderPtr, tmp_len, tmpAllInfoHeaderReadPtr) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
        ALOGD("%s, check panel all info data(0x%x), save the panel all info data.\n", __FUNCTION__, pHeadPtr->data_len);
        SavePanelAllInfoData(pHeadPtr->data_len, tmpAllInfoHeaderPtr);
    }

#if 0 //(defined CC_COMPILE_IN_PC)
    if (panel_id >= 0 && panel_id < panel_cfg_cnt - 1) {
        export_panel_all_one_item(panel_id + 1, tmpAllParseDataPtr);
    } else {
        export_panel_all_one_item(0, tmpAllParseDataPtr);
    }
#endif

    free(tmpAllInfoHeaderPtr);
    tmpAllInfoHeaderPtr = NULL;

    free(tmpAllInfoHeaderReadPtr);
    tmpAllInfoHeaderReadPtr = NULL;


    free(tmpAllInfoDataPtr);
    tmpAllInfoDataPtr = NULL;

    free(pInfoPtr);
    pInfoPtr = NULL;

    return 0;
}

static int check_param_valid(int mode, int parse_len, unsigned char parse_buf[], int ori_len, unsigned char ori_buf[]) {
    unsigned int ori_cal_crc32 = 0, parse_cal_crc32 = 0;

    if (mode == 0) {
        // start check parse data valid
        //ALOGD("%s, start check parse data valid\n", __FUNCTION__);
        if(check_hex_data_have_header_valid(&parse_cal_crc32, CC_MAX_DATA_SIZE, parse_len, parse_buf) < 0) {
            return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;
        }

        // start check flash key data valid
        //ALOGD("%s, start check flash key data valid\n", __FUNCTION__);
        if(check_hex_data_have_header_valid(&ori_cal_crc32, CC_MAX_DATA_SIZE, ori_len, ori_buf) < 0) {
            return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
        }

        if (parse_cal_crc32 != ori_cal_crc32) {
            //ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __FUNCTION__, parse_cal_crc32, ori_cal_crc32);
            return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
        }
        // end check parse data valid
    } else {
        // start check parse data valid
        //ALOGD("%s, start check parse data valid\n", __FUNCTION__);
        if(check_string_data_have_header_valid(&parse_cal_crc32, (char *)parse_buf, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN) < 0) {
            return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;
        }

        // start check flash key data valid
        //ALOGD("%s, start check flash key data valid\n", __FUNCTION__);
        if(check_string_data_have_header_valid(&ori_cal_crc32, (char *)ori_buf, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN) < 0) {
            return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
        }

        if (parse_cal_crc32 != ori_cal_crc32) {
            //ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __FUNCTION__, parse_cal_crc32, ori_cal_crc32);
            return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
        }
        // end check parse data valid
    }

    //ALOGD("%s, param check ok!\n", __FUNCTION__);
    return CC_PARAM_CHECK_OK;
}

static int handle_integrity_flag(void) {
    const char *ini_value = NULL;

    ini_value = IniGetString("start", "start_tag", "null");
    ITEM_LOGD("%s, start_tag is (%s)\n", __FUNCTION__, ini_value);
    if (strcasecmp(ini_value, "amlogic_start")) {
        ALOGE("%s, start_tag (%s) is error!!!\n", __FUNCTION__, ini_value);
        return -1;
    }

    ini_value = IniGetString("end", "end_tag", "null");
    ITEM_LOGD("%s, end_tag is (%s)\n", __FUNCTION__, ini_value);
    if (strcasecmp(ini_value, "amlogic_end")) {
        ITEM_LOGE("%s, end_tag (%s) is error!!!\n", __FUNCTION__, ini_value);
        return -1;
    }

    return 0;
}

static int handle_lcd_basic(struct lcd_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("lcd_Attr", "model_name", "null");
    ITEM_LOGD("%s, model_name is (%s)\n", __FUNCTION__, ini_value);
    strncpy(p_attr->basic.model_name, ini_value, CC_LCD_NAME_LEN_MAX);

    ini_value = IniGetString("lcd_Attr", "interface", "null");
    ITEM_LOGD("%s, interface is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "LCD_TTL") == 0) {
        p_attr->basic.lcd_type = LCD_TTL;
    } else if (strcmp(ini_value, "LCD_LVDS") == 0) {
        p_attr->basic.lcd_type = LCD_LVDS;
    } else if (strcmp(ini_value, "LCD_VBYONE") == 0) {
        p_attr->basic.lcd_type = LCD_VBYONE;
    } else if (strcmp(ini_value, "LCD_MIPI") == 0) {
        p_attr->basic.lcd_type = LCD_MIPI;
    } else if (strcmp(ini_value, "LCD_EDP") == 0) {
        p_attr->basic.lcd_type = LCD_EDP;
    } else {
        p_attr->basic.lcd_type = LCD_VBYONE;
    }

    ini_value = IniGetString("lcd_Attr", "lcd_bits", "10");
    ITEM_LOGD("%s, lcd_bits is (%s)\n", __FUNCTION__, ini_value);
    p_attr->basic.lcd_bits = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "screen_width", "16");
    ITEM_LOGD("%s, screen_width is (%s)\n", __FUNCTION__, ini_value);
    p_attr->basic.screen_width = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "screen_height", "9");
    ITEM_LOGD("%s, screen_height is (%s)\n", __FUNCTION__, ini_value);
    p_attr->basic.screen_height = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_lcd_timming(struct lcd_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("lcd_Attr", "h_active", "3840");
    ITEM_LOGD("%s, h_active is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.h_active = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "v_active", "2160");
    ITEM_LOGD("%s, v_active is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.v_active = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "h_period", "4400");
    ITEM_LOGD("%s, h_period is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.h_period = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "v_period", "2250");
    ITEM_LOGD("%s, v_period is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.v_period = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "hsync_width", "33");
    ITEM_LOGD("%s, hsync_width is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.hsync_width = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "hsync_bp", "477");
    ITEM_LOGD("%s, hsync_bp is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.hsync_bp = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "hsync_pol", "0");
    ITEM_LOGD("%s, hsync_pol is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.hsync_pol = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "vsync_width", "6");
    ITEM_LOGD("%s, vsync_width is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.vsync_width = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "vsync_bp", "65");
    ITEM_LOGD("%s, vsync_bp is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.vsync_bp = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "vsync_pol", "0");
    ITEM_LOGD("%s, vsync_pol is (%s)\n", __FUNCTION__, ini_value);
    p_attr->timming.vsync_pol = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_lcd_customer(struct lcd_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("lcd_Attr", "fr_adjust_type", "0");
    ITEM_LOGD("%s, fr_adjust_type is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.fr_adjust_type = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "ss_level", "0");
    ITEM_LOGD("%s, ss_level is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.ss_level = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "clk_auto_gen", "1");
    ITEM_LOGD("%s, clk_auto_gen is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.clk_auto_gen = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "pixle_clk", "0");
    ITEM_LOGD("%s, pixle_clk is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.pixle_clk = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "h_period_min", "0");
    ITEM_LOGD("%s, h_period_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.h_period_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "h_period_max", "0");
    ITEM_LOGD("%s, h_period_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.h_period_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "v_period_min", "0");
    ITEM_LOGD("%s, v_period_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.v_period_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "v_period_max", "0");
    ITEM_LOGD("%s, v_period_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.v_period_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "pixle_clk_min", "0");
    ITEM_LOGD("%s, pixle_clk_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.pixle_clk_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "pixle_clk_max", "0");
    ITEM_LOGD("%s, pixle_clk_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.pixle_clk_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "customer_value_8", "0");
    ITEM_LOGD("%s, customer_value_8 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.customer_value_8 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "customer_value_9", "0");
    ITEM_LOGD("%s, customer_value_9 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->customer.customer_value_9 = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_lcd_interface(struct lcd_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("lcd_Attr", "if_attr_0", "0");
    ITEM_LOGD("%s, if_attr_0 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_0 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_1", "0");
    ITEM_LOGD("%s, if_attr_1 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_1 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_2", "0");
    ITEM_LOGD("%s, if_attr_2 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_2 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_3", "0");
    ITEM_LOGD("%s, if_attr_3 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_3 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_4", "0");
    ITEM_LOGD("%s, if_attr_4 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_4 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_5", "0");
    ITEM_LOGD("%s, if_attr_5 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_5 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_6", "0");
    ITEM_LOGD("%s, if_attr_6 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_6 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_7", "0");
    ITEM_LOGD("%s, if_attr_7 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_7 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_8", "0");
    ITEM_LOGD("%s, if_attr_8 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_8 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_Attr", "if_attr_9", "0");
    ITEM_LOGD("%s, if_attr_9 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->interface.if_attr_9 = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_lcd_pwr(struct lcd_attr_s* p_attr) {
    int i = 0, tmp_cnt = 0, tmp_base_ind = 0;
    const char *ini_value = NULL;
    unsigned int tmp_buf[1024];

    ini_value = IniGetString("lcd_Attr", "power_on_step", "null");
    ITEM_LOGD("%s, power_on_step is (%s)\n", __FUNCTION__, ini_value);
    tmp_cnt = transBufferData(ini_value, tmp_buf + 0);
    g_lcd_pwr_on_seq_cnt = tmp_cnt / CC_LCD_PWR_ITEM_CNT;
    for (i = 0; i < g_lcd_pwr_on_seq_cnt; i++) {
        tmp_base_ind = i * CC_LCD_PWR_ITEM_CNT;
        p_attr->pwr[i].pwr_step_type = tmp_buf[tmp_base_ind + 0];
        p_attr->pwr[i].pwr_step_index = tmp_buf[tmp_base_ind + 1];
        p_attr->pwr[i].pwr_step_val = tmp_buf[tmp_base_ind + 2];
        p_attr->pwr[i].pwr_step_delay = tmp_buf[tmp_base_ind + 3];
    }

    ini_value = IniGetString("lcd_Attr", "power_off_step", "null");
    ITEM_LOGD("%s, power_off_step is (%s)\n", __FUNCTION__, ini_value);
    tmp_cnt = transBufferData(ini_value, tmp_buf + tmp_cnt);
    g_lcd_pwr_off_seq_cnt = tmp_cnt / CC_LCD_PWR_ITEM_CNT;
    for (i = 0; i < g_lcd_pwr_off_seq_cnt; i++) {
        tmp_base_ind = (g_lcd_pwr_on_seq_cnt + i)* CC_LCD_PWR_ITEM_CNT;
        p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_type = tmp_buf[tmp_base_ind + 0];
        p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_index = tmp_buf[tmp_base_ind + 1];
        p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_val = tmp_buf[tmp_base_ind + 2];
        p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_delay = tmp_buf[tmp_base_ind + 3];
    }

    return 0;
}

static int handle_lcd_header(struct lcd_attr_s* p_attr) {
    const char *ini_value = NULL;

    gLcdDataCnt = 0;
    gLcdDataCnt += sizeof(struct lcd_header_s);
    gLcdDataCnt += sizeof(struct lcd_basic_s);
    gLcdDataCnt += sizeof(struct lcd_timming_s);
    gLcdDataCnt += sizeof(struct lcd_customer_s);
    gLcdDataCnt += sizeof(struct lcd_interface_s);

    gLcdDataCnt += sizeof(struct lcd_pwr_s) * g_lcd_pwr_on_seq_cnt;
    gLcdDataCnt += sizeof(struct lcd_pwr_s) * g_lcd_pwr_off_seq_cnt;

    p_attr->head.data_len = gLcdDataCnt;

    ini_value = IniGetString("lcd_Attr", "version", "null");
    ITEM_LOGD("%s, version is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0) {
        p_attr->head.version = 1;
    } else {
        p_attr->head.version = strtoul(ini_value, NULL, 0);
    }

    p_attr->head.rev = 0;
    p_attr->head.crc32 = CalCRC32(0, (((unsigned char *)p_attr) + 4), gLcdDataCnt - 4);

    return 0;
}

static int handle_lcd_ext_basic(struct lcd_ext_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("lcd_ext_Attr", "ext_name", "null");
    ITEM_LOGD("%s, ext_name is (%s)\n", __FUNCTION__, ini_value);
    strncpy(p_attr->basic.ext_name, ini_value, CC_LCD_EXT_NAME_LEN_MAX);

    ini_value = IniGetString("lcd_ext_Attr", "ext_index", "null");
    ITEM_LOGD("%s, ext_index is (%s)\n", __FUNCTION__, ini_value);
    p_attr->basic.ext_index = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "ext_type", "null");
    ITEM_LOGD("%s, ext_type is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "LCD_EXTERN_I2C") == 0) {
        p_attr->basic.ext_type = LCD_EXTERN_I2C;
    } else if (strcmp(ini_value, "LCD_EXTERN_SPI") == 0) {
        p_attr->basic.ext_type = LCD_EXTERN_SPI;
    } else if (strcmp(ini_value, "LCD_EXTERN_MIPI") == 0) {
        p_attr->basic.ext_type = LCD_EXTERN_MIPI;
    } else {
        p_attr->basic.ext_type = LCD_EXTERN_I2C;
    }

    ini_value = IniGetString("lcd_ext_Attr", "ext_status", "null");
    ITEM_LOGD("%s, ext_status is (%s)\n", __FUNCTION__, ini_value);
    p_attr->basic.ext_status = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_lcd_ext_cmd_type(struct lcd_ext_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("lcd_ext_Attr", "value_0", "null");
    ITEM_LOGD("%s, value_0 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_0 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_1", "null");
    ITEM_LOGD("%s, value_1 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_1 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_2", "null");
    ITEM_LOGD("%s, value_2 is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "LCD_EXTERN_I2C_BUS_AO") == 0) {
        p_attr->cmd_type.value_2 = LCD_EXTERN_I2C_BUS_AO;
    } else if (strcmp(ini_value, "LCD_EXTERN_I2C_BUS_A") == 0) {
        p_attr->cmd_type.value_2 = LCD_EXTERN_I2C_BUS_A;
    } else if (strcmp(ini_value, "LCD_EXTERN_I2C_BUS_B") == 0) {
        p_attr->cmd_type.value_2 = LCD_EXTERN_I2C_BUS_B;
    } else if (strcmp(ini_value, "LCD_EXTERN_I2C_BUS_C") == 0) {
        p_attr->cmd_type.value_2 = LCD_EXTERN_I2C_BUS_C;
    } else if (strcmp(ini_value, "LCD_EXTERN_I2C_BUS_D") == 0) {
        p_attr->cmd_type.value_2 = LCD_EXTERN_I2C_BUS_D;
    } else {
        p_attr->cmd_type.value_2 = LCD_EXTERN_I2C_BUS_D;
    }

    ini_value = IniGetString("lcd_ext_Attr", "value_3", "null");
    ITEM_LOGD("%s, value_3 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_3 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_4", "null");
    ITEM_LOGD("%s, value_4 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_4 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_5", "null");
    ITEM_LOGD("%s, value_5 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_5 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_6", "null");
    ITEM_LOGD("%s, value_6 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_6 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_7", "null");
    ITEM_LOGD("%s, value_7 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_7 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_8", "null");
    ITEM_LOGD("%s, value_8 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_8 = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("lcd_ext_Attr", "value_9", "null");
    ITEM_LOGD("%s, value_9 is (%s)\n", __FUNCTION__, ini_value);
    p_attr->cmd_type.value_9 = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_lcd_ext_cmd_data(struct lcd_ext_attr_s* p_attr) {
    int i = 0, j = 0, tmp_cnt = 0, tmp_base_ind = 0;
    const char *ini_value = NULL;
    unsigned int tmp_buf[2048];

    if (p_attr->cmd_type.value_3 <= 0) {
        return 0;
    }

    ini_value = IniGetString("lcd_ext_Attr", "init_on", "null");
    ITEM_LOGD("%s, init_on is (%s)\n", __FUNCTION__, ini_value);
    tmp_cnt = transBufferData(ini_value, tmp_buf + 0);
    gLcdExtInitOnCnt = tmp_cnt / p_attr->cmd_type.value_3;
    for (i = 0; i < gLcdExtInitOnCnt; i++) {
        tmp_base_ind = i * p_attr->cmd_type.value_3;
        p_attr->cmd_data[i].init_type = tmp_buf[tmp_base_ind + 0];
        for (j = 0; j < p_attr->cmd_type.value_3 - 2; j++) {
            p_attr->cmd_data[i].init_value[j] = tmp_buf[tmp_base_ind + j + 1];
        }
        p_attr->cmd_data[i].init_delay = tmp_buf[tmp_base_ind + p_attr->cmd_type.value_3 - 1];
    }

    ini_value = IniGetString("lcd_ext_Attr", "init_off", "null");
    ITEM_LOGD("%s, init_off is (%s)\n", __FUNCTION__, ini_value);
    tmp_cnt = transBufferData(ini_value, tmp_buf + tmp_cnt);
    gLcdExtInitOffCnt = tmp_cnt / p_attr->cmd_type.value_3;
    for (i = 0; i < gLcdExtInitOffCnt; i++) {
        tmp_base_ind = (gLcdExtInitOnCnt + i) * p_attr->cmd_type.value_3;
        p_attr->cmd_data[i + gLcdExtInitOnCnt].init_type = tmp_buf[tmp_base_ind + 0];
        for (j = 0; j < p_attr->cmd_type.value_3 - 2; j++) {
            p_attr->cmd_data[i + gLcdExtInitOnCnt].init_value[j] = tmp_buf[tmp_base_ind + j + 1];
        }
        p_attr->cmd_data[i + gLcdExtInitOnCnt].init_delay = tmp_buf[tmp_base_ind + p_attr->cmd_type.value_3 - 1];
    }

#if 0
    for (i = 0; i < gLcdExtInitOnCnt; i++) {
        ALOGD("%s, cmd_data[%d].init_type = 0x%02x\n", __FUNCTION__, i, p_attr->cmd_data[i].init_type);
        for (j = 0; j < p_attr->cmd_type.value_3 - 2; j++) {
            ALOGD("%s, cmd_data[%d].init_value[%d] = 0x%02x\n", __FUNCTION__, i, j, p_attr->cmd_data[i].init_value[j]);
        }
        ALOGD("%s, cmd_data[%d].init_delay = 0x%02x\n", __FUNCTION__, i, p_attr->cmd_data[i].init_delay);
    }

    for (i = 0; i < gLcdExtInitOffCnt; i++) {
        ALOGD("%s, cmd_data[%d].init_type = 0x%02x\n", __FUNCTION__, i, p_attr->cmd_data[i + gLcdExtInitOnCnt].init_type);
        for (j = 0; j < p_attr->cmd_type.value_3 - 2; j++) {
            ALOGD("%s, cmd_data[%d].init_value[%d] = 0x%02x\n", __FUNCTION__, i, j, p_attr->cmd_data[i + gLcdExtInitOnCnt].init_value[j]);
        }
        ALOGD("%s, cmd_data[%d].init_delay = 0x%02x\n", __FUNCTION__, i, p_attr->cmd_data[i + gLcdExtInitOnCnt].init_delay);
    }
#endif

    return 0;
}

static int lcd_ext_cmd_data_to_buf(unsigned char tmp_buf[], struct lcd_ext_attr_s* p_attr) {
    int i = 0, j = 0;
    int tmp_len = 0, tmp_off = 0;

    tmp_off = 0;

    tmp_len = sizeof(struct lcd_ext_header_s);
    memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->head), tmp_len);
    tmp_off += tmp_len;

    tmp_len = sizeof(struct lcd_ext_basic_s);
    memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->basic), tmp_len);
    tmp_off += tmp_len;

    tmp_len = sizeof(struct lcd_ext_cmd_type_s);
    memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->cmd_type), tmp_len);
    tmp_off += tmp_len;

    if (p_attr->cmd_type.value_3 > 2) {
        for (i = 0; i < gLcdExtInitOnCnt; i++) {
            tmp_len = i * p_attr->cmd_type.value_3;
            tmp_buf[tmp_off + tmp_len + 0] = p_attr->cmd_data[i].init_type;
            for (j = 0; j < p_attr->cmd_type.value_3 - 2; j++) {
                tmp_buf[tmp_off + tmp_len + j + 1] = p_attr->cmd_data[i].init_value[j];
            }
            tmp_buf[tmp_off + tmp_len + p_attr->cmd_type.value_3 - 1] = p_attr->cmd_data[i].init_delay;
        }

        for (i = 0; i < gLcdExtInitOffCnt; i++) {
            tmp_len = (gLcdExtInitOnCnt + i) * p_attr->cmd_type.value_3;
            tmp_buf[tmp_off + tmp_len + 0] = p_attr->cmd_data[i + gLcdExtInitOnCnt].init_type;
            for (j = 0; j < p_attr->cmd_type.value_3 - 2; j++) {
                tmp_buf[tmp_off + tmp_len + j + 1] = p_attr->cmd_data[i + gLcdExtInitOnCnt].init_value[j];
            }
            tmp_buf[tmp_off + tmp_len + p_attr->cmd_type.value_3 - 1] = p_attr->cmd_data[i + gLcdExtInitOnCnt].init_delay;
        }
    }

    return 0;
}

static int handle_lcd_ext_header(struct lcd_ext_attr_s* p_attr) {
    const char *ini_value = NULL;
    unsigned char *tmp_buf = NULL;

    tmp_buf = (unsigned char *) malloc(CC_MAX_TEMP_BUF_SIZE);
    if (tmp_buf == NULL) {
        ALOGE("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    gLcdExtDataCnt = 0;
    gLcdExtDataCnt += sizeof(struct lcd_ext_header_s);
    gLcdExtDataCnt += sizeof(struct lcd_ext_basic_s);
    gLcdExtDataCnt += sizeof(struct lcd_ext_cmd_type_s);

    gLcdExtDataCnt += p_attr->cmd_type.value_3 * gLcdExtInitOnCnt;
    gLcdExtDataCnt += p_attr->cmd_type.value_3 * gLcdExtInitOffCnt;

    p_attr->head.data_len = gLcdExtDataCnt;

    ini_value = IniGetString("lcd_ext_Attr", "version", "null");
    ITEM_LOGD("%s, version is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0) {
        p_attr->head.version = 1;
    } else {
        p_attr->head.version = strtoul(ini_value, NULL, 0);
    }

    p_attr->head.rev = 0;

    memset((void *)tmp_buf, 0, CC_MAX_TEMP_BUF_SIZE);
    lcd_ext_cmd_data_to_buf(tmp_buf, p_attr);
    p_attr->head.crc32 = CalCRC32(0, (tmp_buf + 4), gLcdExtDataCnt - 4);

    ITEM_LOGD("%s, gLcdExtDataCnt = %d\n", __FUNCTION__, gLcdExtDataCnt);

    free(tmp_buf);
    tmp_buf = NULL;

    return 0;
}

static int handle_bl_basic(struct bl_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("Backlight_Attr", "bl_name", "null");
    ITEM_LOGD("%s, bl_name is (%s)\n", __FUNCTION__, ini_value);
    strncpy(p_attr->basic.bl_name, ini_value, CC_BL_NAME_LEN_MAX);

    return 0;
}

static int handle_bl_level(struct bl_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("Backlight_Attr", "bl_level_uboot", "0");
    ITEM_LOGD("%s, bl_level_uboot is (%s)\n", __FUNCTION__, ini_value);
    p_attr->level.bl_level_uboot = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_level_kernel", "0");
    ITEM_LOGD("%s, bl_level_kernel is (%s)\n", __FUNCTION__, ini_value);
    p_attr->level.bl_level_kernel = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_level_max", "0");
    ITEM_LOGD("%s, bl_level_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->level.bl_level_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_level_min", "0");
    ITEM_LOGD("%s, bl_level_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->level.bl_level_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_level_mid", "0");
    ITEM_LOGD("%s, bl_level_mid is (%s)\n", __FUNCTION__, ini_value);
    p_attr->level.bl_level_mid = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_level_mid_mapping", "0");
    ITEM_LOGD("%s, bl_level_mid_mapping is (%s)\n", __FUNCTION__, ini_value);
    p_attr->level.bl_level_mid_mapping = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_bl_method(struct bl_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("Backlight_Attr", "bl_method", "0");
    ITEM_LOGD("%s, bl_method is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "BL_CTRL_GPIO") == 0) {
        p_attr->method.bl_method = BL_CTRL_GPIO;
    } else if (strcmp(ini_value, "BL_CTRL_PWM") == 0) {
        p_attr->method.bl_method = BL_CTRL_PWM;
    } else if (strcmp(ini_value, "BL_CTRL_PWM_COMBO") == 0) {
        p_attr->method.bl_method = BL_CTRL_PWM_COMBO;
    } else if (strcmp(ini_value, "BL_CTRL_LOCAL_DIMING") == 0) {
        p_attr->method.bl_method = BL_CTRL_LOCAL_DIMING;
    } else if (strcmp(ini_value, "BL_CTRL_EXTERN") == 0) {
        p_attr->method.bl_method = BL_CTRL_EXTERN;
    } else {
        p_attr->method.bl_method = BL_CTRL_PWM_COMBO;
    }

    ini_value = IniGetString("Backlight_Attr", "bl_en_gpio", "0");
    ITEM_LOGD("%s, bl_en_gpio is (%s)\n", __FUNCTION__, ini_value);
    p_attr->method.bl_en_gpio = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_en_gpio_on", "0");
    ITEM_LOGD("%s, bl_en_gpio_on is (%s)\n", __FUNCTION__, ini_value);
    p_attr->method.bl_en_gpio_on = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_en_gpio_off", "0");
    ITEM_LOGD("%s, bl_en_gpio_off is (%s)\n", __FUNCTION__, ini_value);
    p_attr->method.bl_en_gpio_off = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_on_delay", "0");
    ITEM_LOGD("%s, bl_on_delay is (%s)\n", __FUNCTION__, ini_value);
    p_attr->method.bl_on_delay = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "bl_off_delay", "0");
    ITEM_LOGD("%s, bl_off_delay is (%s)\n", __FUNCTION__, ini_value);
    p_attr->method.bl_off_delay = strtoul(ini_value, NULL, 0);

    return 0;
}

static int getPWMMethod(const char* ini_value, int def_val) {
    if (strcmp(ini_value, "BL_PWM_NEGATIVE") == 0) {
        return BL_PWM_NEGATIVE;
    } else if (strcmp(ini_value, "BL_PWM_POSITIVE") == 0) {
        return BL_PWM_POSITIVE;
    } else {
        return def_val;
    }
}

static int getPWMPortIndVal(const char* ini_value, int def_val) {
    if (strcmp(ini_value, "BL_PWM_A") == 0) {
        return BL_PWM_A;
    } else if (strcmp(ini_value, "BL_PWM_B") == 0) {
        return BL_PWM_B;
    } else if (strcmp(ini_value, "BL_PWM_C") == 0) {
        return BL_PWM_C;
    } else if (strcmp(ini_value, "BL_PWM_D") == 0) {
        return BL_PWM_D;
    } else if (strcmp(ini_value, "BL_PWM_E") == 0) {
        return BL_PWM_E;
    } else if (strcmp(ini_value, "BL_PWM_F") == 0) {
        return BL_PWM_F;
    } else if (strcmp(ini_value, "BL_PWM_VS") == 0) {
        return BL_PWM_VS;
    } else {
        return def_val;
    }
}

static int handle_bl_pwm(struct bl_attr_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("Backlight_Attr", "pwm_method", "BL_PWM_POSITIVE");
    ITEM_LOGD("%s, pwm_method is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_method = getPWMMethod(ini_value, BL_PWM_POSITIVE);

    ini_value = IniGetString("Backlight_Attr", "pwm_port", "PWM_B");
    ITEM_LOGD("%s, pwm_port is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_port = getPWMPortIndVal(ini_value, BL_PWM_B);

    ini_value = IniGetString("Backlight_Attr", "pwm_freq", "0");
    ITEM_LOGD("%s, pwm_freq is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_freq = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_duty_max", "0");
    ITEM_LOGD("%s, pwm_duty_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_duty_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_duty_min", "0");
    ITEM_LOGD("%s, pwm_duty_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_duty_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_gpio", "0");
    ITEM_LOGD("%s, pwm_gpio is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_gpio = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_gpio_off", "0");
    ITEM_LOGD("%s, pwm_gpio_off is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_gpio_off = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm2_method", "BL_PWM_POSITIVE");
    ITEM_LOGD("%s, pwm2_method is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_method = getPWMMethod(ini_value, BL_PWM_POSITIVE);

    ini_value = IniGetString("Backlight_Attr", "pwm2_port", "PWM_D");
    ITEM_LOGD("%s, pwm2_port is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_port = getPWMPortIndVal(ini_value, BL_PWM_D);

    ini_value = IniGetString("Backlight_Attr", "pwm2_freq", "0");
    ITEM_LOGD("%s, pwm2_freq is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_freq = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm2_duty_max", "0");
    ITEM_LOGD("%s, pwm2_duty_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_duty_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm2_duty_min", "0");
    ITEM_LOGD("%s, pwm2_duty_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_duty_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm2_gpio", "0");
    ITEM_LOGD("%s, pwm2_gpio is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_gpio = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm2_gpio_off", "0");
    ITEM_LOGD("%s, pwm2_gpio_off is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_gpio_off = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_on_delay", "0");
    ITEM_LOGD("%s, pwm_on_delay is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_on_delay = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_off_delay", "0");
    ITEM_LOGD("%s, pwm_off_delay is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_off_delay = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_level_max", "0");
    ITEM_LOGD("%s, pwm_level_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_level_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm_level_min", "0");
    ITEM_LOGD("%s, pwm_level_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm_level_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm2_level_max", "0");
    ITEM_LOGD("%s, pwm2_level_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_level_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("Backlight_Attr", "pwm2_level_min", "0");
    ITEM_LOGD("%s, pwm2_level_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->pwm.pwm2_level_min = strtoul(ini_value, NULL, 0);

    return 0;
}

static int handle_bl_header(struct bl_attr_s* p_attr) {
    const char *ini_value = NULL;

    gBlDataCnt = 0;
    gBlDataCnt += sizeof(struct bl_header_s);
    gBlDataCnt += sizeof(struct bl_basic_s);
    gBlDataCnt += sizeof(struct bl_level_s);
    gBlDataCnt += sizeof(struct bl_method_s);
    gBlDataCnt += sizeof(struct bl_pwm_s);

    p_attr->head.data_len = gBlDataCnt;

    ini_value = IniGetString("Backlight_Attr", "version", "null");
    ITEM_LOGD("%s, version is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0) {
        p_attr->head.version = 1;
    } else {
        p_attr->head.version = strtoul(ini_value, NULL, 0);
    }

    p_attr->head.rev = 0;
    p_attr->head.crc32 = CalCRC32(0, (((unsigned char *)p_attr) + 4), gBlDataCnt - 4);

    return 0;
}

static int handle_panel_misc(struct panel_misc_s* p_misc) {
    int tmp_val = 0;
    const char *ini_value = NULL;
    char buf[128] = {0};

    ini_value = IniGetString("panel_misc", "panel_misc_version", "null");
    ITEM_LOGD("%s, panel_misc_version is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0) {
        strcpy(p_misc->version, "V001");
    } else {
        tmp_val = strtol(ini_value, NULL, 0);
        if (tmp_val < 1) {
            tmp_val = 1;
        }

        sprintf(p_misc->version, "V%03d", tmp_val);
    }

    ini_value = IniGetString("panel_misc", "panel_reverse", "null");
    ITEM_LOGD("%s, panel_reverse is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0 || strcmp(ini_value, "0") == 0 ||
        strcmp(ini_value, "false") == 0 || strcmp(ini_value, "no_rev") == 0) {
        strcpy(p_misc->reverse, "no_rev");
    } else if (strcmp(ini_value, "true") == 0 || strcmp(ini_value, "1") == 0 ||
        strcmp(ini_value, "have_rev") == 0) {
        strcpy(p_misc->reverse, "have_rev");
    } else {
        strcpy(p_misc->reverse, "no_rev");
    }

    ini_value = IniGetString("panel_misc", "outputmode", "null");
    ITEM_LOGD("%s, outputmode is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0) {
        strcpy(p_misc->outputmode, "2160p60hz");
    } else {
        strcpy(p_misc->outputmode, ini_value);
    }

    ini_value = IniGetString("panel_misc", "osd_reverse", "null");
    ITEM_LOGD("%s, osd_reverse is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0) {
        strcpy(p_misc->osd_reverse, "0");
    } else {
        strcpy(p_misc->osd_reverse, ini_value);
    }

    ini_value = IniGetString("panel_misc", "video_reverse", "null");
    ITEM_LOGD("%s, video_reverse is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "null") == 0) {
        strcpy(p_misc->video_reverse, "0");
    } else {
        strcpy(p_misc->video_reverse, ini_value);
    }

    ini_value = IniGetString("panel_misc", "vcomTuning", "null");
    printf("%s, vcomTuning is (%s)\n", __FUNCTION__, ini_value);
    if (strcmp(ini_value, "yes") == 0) {
        setWpPin();
    }

    sprintf(buf, "setenv outputmode %s", p_misc->outputmode);
    run_command(buf, 0);
    sprintf(buf, "setenv osd_reverse %s", p_misc->osd_reverse);
    run_command(buf, 0);
    sprintf(buf, "setenv video_reverse %s", p_misc->video_reverse);
    run_command(buf, 0);
    return 0;
}

#if (defined CC_COMPILE_IN_PC)
static const char* getPWMPortStrVal(int pwm_port, const char *def_str) {
    if (pwm_port == BL_PWM_A) {
        return "PWM_A";
    } else if (pwm_port == BL_PWM_B) {
        return "PWM_B";
    } else if (pwm_port == BL_PWM_C) {
        return "PWM_C";
    } else if (pwm_port == BL_PWM_D) {
        return "PWM_D";
    } else if (pwm_port == BL_PWM_E) {
        return "PWM_E";
    } else if (pwm_port == BL_PWM_F) {
        return "PWM_F";
    } else if (pwm_port == BL_PWM_VS) {
        return "PWM_VS";
    } else {
        return def_str;
    }
}

static int ExportDataAsDts(const char *fname_str, struct lcd_attr_s* p_lcd_attr, struct lcd_ext_attr_s *p_lcd_ext_attr, struct bl_attr_s* p_bl_attr) {
    int i = 0, tmp_val = 0;
    FILE* fp = stderr;

    if (p_lcd_attr == NULL || p_bl_attr == NULL) {
        return -1;
    }

    if (fname_str != NULL) {
        fp = fopen(fname_str, "w");
    }

    // start export lcd attr
    fprintf(fp, "\t\t\tmodel_name = \"%s\";\n", p_lcd_attr->basic.model_name);

    if (p_lcd_attr->basic.lcd_type == LCD_TTL) {
        fprintf(fp, "\t\t\tinterface = \"%s\"; /* lcd_interface(lvds, vbyone) */\n", "ttl");
    } else if (p_lcd_attr->basic.lcd_type == LCD_LVDS) {
        fprintf(fp, "\t\t\tinterface = \"%s\"; /* lcd_interface(lvds, vbyone) */\n", "lvds");
    } else if (p_lcd_attr->basic.lcd_type == LCD_VBYONE) {
        fprintf(fp, "\t\t\tinterface = \"%s\"; /* lcd_interface(lvds, vbyone) */\n", "vbyone");
    } else if (p_lcd_attr->basic.lcd_type == LCD_MIPI) {
        fprintf(fp, "\t\t\tinterface = \"%s\"; /* lcd_interface(lvds, vbyone) */\n", "mipi");
    } else if (p_lcd_attr->basic.lcd_type == LCD_EDP) {
        fprintf(fp, "\t\t\tinterface = \"%s\"; /* lcd_interface(lvds, vbyone) */\n", "edp");
    }

    fprintf(fp, "\t\t\tbasic_setting = <%d %d %d %d %d %d %d>; /* h_active, v_active, h_period, v_period, lcd_bits, screen_widht, screen_height */\n",
            p_lcd_attr->timming.h_active, p_lcd_attr->timming.v_active,
            p_lcd_attr->timming.h_period, p_lcd_attr->timming.v_period,
            p_lcd_attr->basic.lcd_bits, p_lcd_attr->basic.screen_width,
            p_lcd_attr->basic.screen_height);

    fprintf(fp, "\t\t\tlcd_timing = <%d %d %d %d %d %d>; /* hs_width, hs_bp, hs_pol, vs_width, vs_bp, vs_pol */\n",
            p_lcd_attr->timming.hsync_width, p_lcd_attr->timming.hsync_bp,
            p_lcd_attr->timming.hsync_pol, p_lcd_attr->timming.vsync_width,
            p_lcd_attr->timming.vsync_bp, p_lcd_attr->timming.vsync_pol);

    fprintf(fp, "\t\t\tclk_attr = <%d %d %d %d>; /* fr_adj_type(0=clock, 1=htotal, 2=vtotal), clk_ss_level, clk_auto_generate, pixel_clk(unit in Hz) */\n",
            p_lcd_attr->customer.fr_adjust_type, p_lcd_attr->customer.ss_level,
            p_lcd_attr->customer.clk_auto_gen, p_lcd_attr->customer.pixle_clk);

    fprintf(fp, "\t\t\tvbyone_attr = <%d %d %d %d>; /* lane_count, region_num, byte_mode, color_fmt */\n",
            p_lcd_attr->interface.if_attr_0, p_lcd_attr->interface.if_attr_1,
            p_lcd_attr->interface.if_attr_2, p_lcd_attr->interface.if_attr_3);

    fprintf(fp, "\t\t\tphy_attr=<%d %d>; /* vswing_level, preemphasis_level */\n",
            p_lcd_attr->interface.if_attr_4, p_lcd_attr->interface.if_attr_5);

    for (i = 0; i < g_lcd_pwr_on_seq_cnt; i++) {
        if (i == 0) {
            fprintf(fp, "\t\t\tpower_on_step = <%d %d %d %d\n",
                    p_lcd_attr->pwr[i].pwr_step_type, p_lcd_attr->pwr[i].pwr_step_index,
                    p_lcd_attr->pwr[i].pwr_step_val, p_lcd_attr->pwr[i].pwr_step_delay);
        } else if (i < g_lcd_pwr_on_seq_cnt - 1) {
            fprintf(fp, "\t\t\t\t\t%d %d %d %d\n",
                    p_lcd_attr->pwr[i].pwr_step_type, p_lcd_attr->pwr[i].pwr_step_index,
                    p_lcd_attr->pwr[i].pwr_step_val, p_lcd_attr->pwr[i].pwr_step_delay);
        } else {
            fprintf(fp, "\t\t\t\t\t0x%x %d %d %d>; /* type, index, value, delay */\n",
                    p_lcd_attr->pwr[i].pwr_step_type, p_lcd_attr->pwr[i].pwr_step_index,
                    p_lcd_attr->pwr[i].pwr_step_val, p_lcd_attr->pwr[i].pwr_step_delay);
        }
    }

    for (i = 0; i < g_lcd_pwr_off_seq_cnt; i++) {
        if (i == 0) {
            fprintf(fp, "\t\t\tpower_off_step = <%d %d %d %d\n",
                    p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_type, p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_index,
                    p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_val, p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_delay);
        } else if (i < g_lcd_pwr_off_seq_cnt - 1) {
            fprintf(fp, "\t\t\t\t\t%d %d %d %d\n",
                    p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_type, p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_index,
                    p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_val, p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_delay);
        } else {
            fprintf(fp, "\t\t\t\t\t0x%x %d %d %d>; /* type, index, value, delay */\n",
                    p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_type, p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_index,
                    p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_val, p_lcd_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_delay);
        }
    }
    // end export lcd attr

    fprintf(fp, "\n\n\n");

    // start export lcd extern attr
    fprintf(fp, "\t\t\tindex = <%d>;\n", p_lcd_ext_attr->basic.ext_index);
    fprintf(fp, "\t\t\textern_name = \"%s\";\n", p_lcd_ext_attr->basic.ext_name);
    if (p_lcd_ext_attr->basic.ext_status) {
        fprintf(fp, "\t\t\tstatus = \"%s\";\n", "okay");
    } else {
        fprintf(fp, "\t\t\tstatus = \"%s\";\n", "disable");
    }

    fprintf(fp, "\t\t\ttype = <%d>; /* 0=i2c, 1=spi, 2=mipi */\n", p_lcd_ext_attr->basic.ext_type);

    fprintf(fp, "\t\t\ti2c_address = <0x%x>; /* 7bit i2c address */\n", p_lcd_ext_attr->cmd_type.value_0);
    fprintf(fp, "\t\t\ti2c_second_address = <0x%x>; /* 7bit i2c address, 0xff for none */\n", p_lcd_ext_attr->cmd_type.value_1);
    if (p_lcd_ext_attr->cmd_type.value_2 == LCD_EXTERN_I2C_BUS_AO) {
        fprintf(fp, "\t\t\ti2c_bus = \"%s\";\n", "i2c_bus_ao");
    } else if (p_lcd_ext_attr->cmd_type.value_2 == LCD_EXTERN_I2C_BUS_A) {
        fprintf(fp, "\t\t\ti2c_bus = \"%s\";\n", "i2c_bus_a");
    } else if (p_lcd_ext_attr->cmd_type.value_2 == LCD_EXTERN_I2C_BUS_B) {
        fprintf(fp, "\t\t\ti2c_bus = \"%s\";\n", "i2c_bus_b");
    } else if (p_lcd_ext_attr->cmd_type.value_2 == LCD_EXTERN_I2C_BUS_C) {
        fprintf(fp, "\t\t\ti2c_bus = \"%s\";\n", "i2c_bus_c");
    } else if (p_lcd_ext_attr->cmd_type.value_2 == LCD_EXTERN_I2C_BUS_D) {
        fprintf(fp, "\t\t\ti2c_bus = \"%s\";\n", "i2c_bus_d");
    }

    fprintf(fp, "\t\t\tcmd_size = <%d>;\n", p_lcd_ext_attr->cmd_type.value_3);

    fprintf(fp, "\t\t\t/* init on/off: (type, value..., delay), must match cmd_size for every group */\n");
    fprintf(fp, "\t\t\t/* type: 0x00=gpio, 0x10=cmd(bit[3:0]=1 for second_addr), 0xff=ending*/\n");
    fprintf(fp, "\t\t\t/* value: i2c or spi cmd, or gpio index & level, fill 0x0 for no use */\n");
    fprintf(fp, "\t\t\t/* delay: unit ms */\n");

    for (i = 0; i < gLcdExtInitOnCnt * p_lcd_ext_attr->cmd_type.value_3; i++) {
        if (i % p_lcd_ext_attr->cmd_type.value_3 == 0) {
            if (i == 0) {
                fprintf(fp, "\t\t\tinit_on = <0x%02X ", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3].init_type);
            } else {
                fprintf(fp, "\t\t\t\t0x%02X ", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3].init_type);
            }
        } else if (i % p_lcd_ext_attr->cmd_type.value_3 == p_lcd_ext_attr->cmd_type.value_3 - 1) {
            if (i + 1 == gLcdExtInitOnCnt * p_lcd_ext_attr->cmd_type.value_3) {
                fprintf(fp, "0x%02X>;\n", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3].init_delay);
            } else {
                fprintf(fp, "0x%02X\n", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3].init_delay);
            }
        } else {
            fprintf(fp, "0x%02X ", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3].init_value[i % p_lcd_ext_attr->cmd_type.value_3 - 1]);
        }
    }

    for (i = 0; i < gLcdExtInitOffCnt * p_lcd_ext_attr->cmd_type.value_3; i++) {
        if (i % p_lcd_ext_attr->cmd_type.value_3 == 0) {
            if (i == 0) {
                fprintf(fp, "\t\t\tinit_off = <0x%02X ", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3 + gLcdExtInitOnCnt].init_type);
            } else {
                fprintf(fp, "\t\t\t\t0x%02X ", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3 + gLcdExtInitOnCnt].init_type);
            }
        } else if (i % p_lcd_ext_attr->cmd_type.value_3 == p_lcd_ext_attr->cmd_type.value_3 - 1) {
            if (i + 1 == gLcdExtInitOffCnt * p_lcd_ext_attr->cmd_type.value_3) {
                fprintf(fp, "0x%02X>;\n", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3 + gLcdExtInitOnCnt].init_delay);
            } else {
                fprintf(fp, "0x%02X\n", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3 + gLcdExtInitOnCnt].init_delay);
            }
        } else {
            fprintf(fp, "0x%02X ", p_lcd_ext_attr->cmd_data[i / p_lcd_ext_attr->cmd_type.value_3 + gLcdExtInitOnCnt].init_value[i % p_lcd_ext_attr->cmd_type.value_3 - 1]);
        }
    }
    // end export lcd extern attr

    fprintf(fp, "\n\n\n");

    // start export backlight attr
    fprintf(fp, "\t\t\tbl_name = \"%s\";\n", p_bl_attr->basic.bl_name);

    fprintf(fp, "\t\t\tbl_level_default_uboot_kernel = <%d 0x%x>;\n",
            p_bl_attr->level.bl_level_uboot, p_bl_attr->level.bl_level_kernel);

    fprintf(fp, "\t\t\tbl_level_attr = <%d %d %d %d>; /* max, min, mid, mid_mapping */\n",
            p_bl_attr->level.bl_level_max, p_bl_attr->level.bl_level_min,
            p_bl_attr->level.bl_level_mid, p_bl_attr->level.bl_level_mid_mapping);

    fprintf(fp, "\t\t\tbl_ctrl_method = <%d>; /* 0=gpio, 1=pwm, 2=pwm_combo, 3=ldim, 4=extern */\n", p_bl_attr->method.bl_method);

    fprintf(fp, "\t\t\tbl_power_attr = <%d %d %d %d %d>; /* en_gpio_index, on_value, off_value, on_delay, off_delay */\n",
            p_bl_attr->method.bl_en_gpio, p_bl_attr->method.bl_en_gpio_on,
            p_bl_attr->method.bl_en_gpio_off, p_bl_attr->method.bl_on_delay,
            p_bl_attr->method.bl_off_delay);


    fprintf(fp, "\t\t\t/* pwm_method: 0=negative, 1=positive */\n");
    fprintf(fp, "\t\t\t/* pwm_freq: pwm_vs: 1~4(vfreq multiple), other pwm: real freq(unit: Hz) */\n");
    fprintf(fp, "\t\t\t/* duty_max, duty_min: unit in %% */\n");

    fprintf(fp, "\t\t\tbl_pwm_combo_level_mapping = <%d %d %d %d>; /* level_max, level_min */\n",
            p_bl_attr->pwm.pwm_level_max, p_bl_attr->pwm.pwm_level_min,
            p_bl_attr->pwm.pwm2_level_max, p_bl_attr->pwm.pwm2_level_min);

    const char* pwm_port_str = getPWMPortStrVal(p_bl_attr->pwm.pwm_port, "PWM_B");
    const char* pwm2_port_str = getPWMPortStrVal(p_bl_attr->pwm.pwm2_port, "PWM_B");
    fprintf(fp, "\t\t\tbl_pwm_combo_port = \"%s\",\"%s\"; /* PWM_A, PWM_B, PWM_C, PWM_D, PWM_VS */\n", pwm_port_str, pwm2_port_str);

    fprintf(fp, "\t\t\tbl_pwm_combo_attr = <%d %d %d %d\n",
            p_bl_attr->pwm.pwm_method, p_bl_attr->pwm.pwm_freq,
            p_bl_attr->pwm.pwm_duty_max, p_bl_attr->pwm.pwm_duty_min);

    fprintf(fp, "\t\t\t\t\t     %d %d %d %d>; /* pwm_method, pwm_freq, duty_max, duty_min */\n",
            p_bl_attr->pwm.pwm2_method, p_bl_attr->pwm.pwm2_freq,
            p_bl_attr->pwm.pwm2_duty_max, p_bl_attr->pwm.pwm2_duty_min);

    fprintf(fp, "\t\t\tbl_pwm_combo_power = <%d %d %d %d %d %d>; /* pwm0_gpio_index, pwm0_gpio_off, pwm1_gpio_index, pwm1_gpio_off, pwm_on_delay, pwm_off_delay */\n",
            p_bl_attr->pwm.pwm_gpio, p_bl_attr->pwm.pwm_gpio_off,
            p_bl_attr->pwm.pwm2_gpio, p_bl_attr->pwm.pwm2_gpio_off,
            p_bl_attr->pwm.pwm_on_delay, p_bl_attr->pwm.pwm_off_delay);
    // end export backlight attr

    fprintf(fp, "\n\n\n");

    if (fname_str != NULL) {
        fclose(fp);
    }

    fp = NULL;

    return 0;
}

static int ExportDataAsBin(const char *fname_str, void *data, int data_len) {
    int i = 0, total_len = 0;
    int fd = -1;

    if (fname_str == NULL || data == NULL || data_len == 0) {
        return -1;
    }

    fd = open(fname_str, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        ALOGE("%s, Open %s ERROR(%s)!!\n", __FUNCTION__, fname_str, strerror(errno));
        return -1;
    }

    write(fd, data, data_len);

    close(fd);
    fd = -1;

    return 0;
}

static int export_panel_all_one_item(int index, unsigned char data_buf[]) {
    struct all_info_header_s *pHeadPtr = NULL;
    struct all_info_item_headers_s *pItemHeadPtr = NULL;

    ALOGD("%s, export index = %d\n", __FUNCTION__, index);

    pHeadPtr = (struct all_info_header_s *)(data_buf + 0);
    pItemHeadPtr = (struct all_info_item_headers_s *)(data_buf + pHeadPtr->item_head_off + index * pHeadPtr->item_head_size);

    ExportDataAsBin("lcd.bin", (void *)(data_buf + pItemHeadPtr->lcd.off), pItemHeadPtr->lcd.len);
    ExportDataAsBin("lcd_ext.bin", (void *)(data_buf + pItemHeadPtr->lcd_ext.off), pItemHeadPtr->lcd_ext.len);
    ExportDataAsBin("backlight.bin", (void *)(data_buf + pItemHeadPtr->backlight.off), pItemHeadPtr->backlight.len);
    ExportDataAsBin("panel_misc.bin", (void *)(data_buf + pItemHeadPtr->panel_misc.off), pItemHeadPtr->panel_misc.len);

    return 0;
}
#endif

static int handle_panel_one_file_data(char *file_name, struct all_info_item_headers_s *head, unsigned char data_buf[]) {
    int i = 0, tmp_off = 0;
    unsigned char *tmp_buf = NULL;
    unsigned char *parse_buf = NULL;
    struct lcd_attr_s lcd_attr;
    struct lcd_ext_attr_s lcd_ext_attr;
    struct bl_attr_s bl_attr;
    struct panel_misc_s misc_attr;

    tmp_buf = (unsigned char *) malloc(CC_MAX_TEMP_BUF_SIZE);
    if (tmp_buf == NULL) {
        ALOGE("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    parse_buf = (unsigned char *) malloc(CC_MAX_TEMP_BUF_SIZE);
    if (parse_buf == NULL) {
        free(tmp_buf);
        tmp_buf = NULL;

        ALOGE("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    ALOGD("%s, start handle panel ini file \"%s\"\n", __FUNCTION__, file_name);

    memset((void *)&lcd_attr, 0, sizeof(struct lcd_attr_s));
    memset((void *)&lcd_ext_attr, 0, sizeof(struct lcd_ext_attr_s));
    memset((void *)&bl_attr, 0, sizeof(struct bl_attr_s));
    memset((void *)&misc_attr, 0, sizeof(struct panel_misc_s));

    //init misc attr as default
    strcpy(misc_attr.version, "V001");
    strcpy(misc_attr.reverse, "no_rev");
    strcpy(misc_attr.outputmode, "2160p60hz");

    if (parse_panel_ini(file_name, &lcd_attr, &lcd_ext_attr, &bl_attr, &misc_attr) < 0) {
        free(tmp_buf);
        tmp_buf = NULL;

        free(parse_buf);
        parse_buf = NULL;
        return 0;
    }

    tmp_off = 0;

    // start handle lcd param
    head->lcd.off = tmp_off;
    head->lcd.len = gLcdDataCnt;
    memcpy(data_buf + tmp_off, (void *)&lcd_attr, head->lcd.len);
    tmp_off += head->lcd.len;
    ALOGD("%s, lcd param size (%d)\n", __FUNCTION__, head->lcd.len);
    // end handle lcd param

    // start handle lcd extern param
    if (lcd_ext_attr.basic.ext_status == 1) {
        memset((void *)parse_buf, 0, CC_MAX_TEMP_BUF_SIZE);
        lcd_ext_cmd_data_to_buf(parse_buf, &lcd_ext_attr);

        head->lcd_ext.off = tmp_off;
        head->lcd_ext.len = gLcdExtDataCnt;
        memcpy(data_buf + tmp_off, parse_buf, head->lcd_ext.len);
        tmp_off += head->lcd_ext.len;
        ALOGD("%s, lcd extern 01 size (%d)\n", __FUNCTION__, head->lcd_ext.len);
    } else {
        head->lcd_ext.off = tmp_off;
        head->lcd_ext.len = gLcdExtDataCnt;

        memset((void *)&lcd_ext_attr, 0, sizeof(struct lcd_ext_attr_s));
        lcd_ext_attr.head.data_len = head->lcd_ext.len;
        lcd_ext_attr.head.crc32 = CalCRC32(0, (((unsigned char *)&lcd_ext_attr) + 4), head->lcd_ext.len - 4);
        memcpy(data_buf + tmp_off, (void *)&lcd_ext_attr, head->lcd_ext.len);
        tmp_off += head->lcd_ext.len;
        ALOGD("%s, lcd extern 02 size (%d)\n", __FUNCTION__, head->lcd_ext.len);
    }
    // end handle lcd extern param

    // start handle backlight param
    head->backlight.off = tmp_off;
    head->backlight.len = gBlDataCnt;
    memcpy(data_buf + tmp_off, (unsigned char*)&bl_attr, head->backlight.len);
    tmp_off += head->backlight.len;
    ALOGD("%s, backlight size (%d)\n", __FUNCTION__, head->backlight.len);
    // end handle backlight param

    // start handle panel misc
    memset((void *)tmp_buf, 0, CC_MAX_TEMP_BUF_SIZE);
    strcpy((char *)tmp_buf, misc_attr.version);
    strcat((char *)tmp_buf, ",");
    strcat((char *)tmp_buf, misc_attr.reverse);
    strcat((char *)tmp_buf, ",");
    strcat((char *)tmp_buf, misc_attr.outputmode);
    unsigned int tmp_crc32 = CalCRC32(0, tmp_buf, strlen((char *)tmp_buf));
    sprintf((char *)parse_buf, "%08x,%s", tmp_crc32, (char *)tmp_buf);

    head->panel_misc.off = tmp_off;
    head->panel_misc.len = strlen((char *)parse_buf) + 1;
    memcpy(data_buf + tmp_off, parse_buf, head->panel_misc.len);
    tmp_off += head->panel_misc.len;
    ALOGD("%s, panel misc size (%d)\n", __FUNCTION__, head->panel_misc.len);
    // end handle panel misc

    // start handle rev
    head->rev1.off = tmp_off;
    head->rev1.len = 0;
    head->rev2.off = tmp_off;
    head->rev2.len = 0;
    // end handle rev

    for (i = 0; i < tmp_off; i++) {
        ITEM_LOGD("%s, data_buf[%d] = 0x%02X\n", __FUNCTION__, i, data_buf[i]);
    }

    ALOGD("%s, tmp_off = %d\n", __FUNCTION__, tmp_off);

    free(tmp_buf);
    tmp_buf = NULL;

    free(parse_buf);
    parse_buf = NULL;

    return tmp_off;
}

int handle_hdr_ini_by_id(int panel_id, char *file_name, struct lcd_hdr_info_s *p_attr) {
    int default_flag = 0, panel_cfg_cnt = 0;
    struct project_info_s *pInfoPtr = NULL;

    pInfoPtr = (struct project_info_s *) malloc(CC_MAX_SUPPORT_PANEL_CNT * sizeof(struct project_info_s));
    if (pInfoPtr == NULL) {
        ALOGE("%s, malloc panel info memory error!!!\n", __FUNCTION__);
        return -1;
    }

    ALOGD("%s, panel_id = %d, panel_info_name is (%s)\n", __FUNCTION__, panel_id, file_name);

    default_flag = CC_HDR_PATH_MASK;
    panel_cfg_cnt = handle_get_project_all_info(file_name, &default_flag, pInfoPtr);
    ALOGD("%s, panel_cfg_cnt = %d, default flag (0x%08x)\n", __FUNCTION__, panel_cfg_cnt, default_flag);
    if (default_flag & CC_HDR_PATH_MASK) {
        panel_cfg_cnt += 1;
    }

    //handle current panel id's hdr ini
    if (panel_cfg_cnt > 0) {
        if (panel_id >= 0 && panel_id < panel_cfg_cnt - 1) {
            ALOGD("%s, start handle current panel id(%d)'s hdr ini.\n", __FUNCTION__, panel_id);

            handle_hdr_ini(pInfoPtr[panel_id].hdr_ini_path, p_attr);
        } else {
            ALOGD("%s, use default hdr ini.\n", __FUNCTION__);

            if (default_flag & CC_HDR_PATH_MASK) {
                handle_hdr_ini(pInfoPtr[panel_cfg_cnt - 1].hdr_ini_path, p_attr);
            } else {
                free(pInfoPtr);
                pInfoPtr = NULL;

                ALOGD("%s, there is no default hdr ini.\n", __FUNCTION__);
                return -1;
            }
        }
    } else {
        free(pInfoPtr);
        pInfoPtr = NULL;

        ALOGD("%s, there is no hdr ini.\n", __FUNCTION__);
        return -1;
    }

    free(pInfoPtr);
    pInfoPtr = NULL;

    return 0;
}

int handle_hdr_ini(char *file_name, struct lcd_hdr_info_s *p_attr) {
    memset((void *)p_attr, 0, sizeof(struct lcd_hdr_info_s));

    IniParserInit();

    if (IniParseFile(file_name) < 0) {
        ALOGE("%s, ini load file error!\n", __FUNCTION__);

        IniParserUninit();
        return -1;
    }

    // handle integrity flag
    if (handle_integrity_flag() < 0) {
        IniParserUninit();
        return -1;
    }

    handle_hdr_attr(p_attr);

    IniParserUninit();

    return 0;
}

static int handle_hdr_attr(struct lcd_hdr_info_s* p_attr) {
    const char *ini_value = NULL;

    ini_value = IniGetString("hdr_Attr", "hdr_support", "0");
    ITEM_LOGD("%s, hdr_support is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_support = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_features", "0");
    ITEM_LOGD("%s, hdr_features is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_features = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_primaries_r_x", "0");
    ITEM_LOGD("%s, hdr_primaries_r_x is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_primaries_r_x = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_primaries_r_y", "0");
    ITEM_LOGD("%s, hdr_primaries_r_y is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_primaries_r_y = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_primaries_g_x", "0");
    ITEM_LOGD("%s, hdr_primaries_g_x is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_primaries_g_x = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_primaries_g_y", "0");
    ITEM_LOGD("%s, hdr_primaries_g_y is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_primaries_g_y = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_primaries_b_x", "0");
    ITEM_LOGD("%s, hdr_primaries_b_x is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_primaries_b_x = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_primaries_b_y", "0");
    ITEM_LOGD("%s, hdr_primaries_b_y is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_primaries_b_y = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_white_point_x", "0");
    ITEM_LOGD("%s, hdr_white_point_x is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_white_point_x = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_white_point_y", "0");
    ITEM_LOGD("%s, hdr_white_point_y is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_white_point_y = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_luma_max", "0");
    ITEM_LOGD("%s, hdr_luma_max is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_luma_max = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_luma_min", "0");
    ITEM_LOGD("%s, hdr_luma_min is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_luma_min = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("hdr_Attr", "hdr_luma_avg", "0");
    ITEM_LOGD("%s, hdr_luma_avg is (%s)\n", __FUNCTION__, ini_value);
    p_attr->hdr_luma_avg = strtoul(ini_value, NULL, 0);

    return 0;
}

static int transBufferData(const char *data_str, unsigned int data_buf[]) {
    int item_ind = 0;
    char *token = NULL;
    char *pSave = NULL;
    char *tmp_buf = NULL;

    if (data_str == NULL) {
        return 0;
    }

    tmp_buf = (char *) malloc(CC_MAX_TEMP_BUF_SIZE);
    if (tmp_buf == NULL) {
        ALOGE("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    memset((void *)tmp_buf, 0, CC_MAX_TEMP_BUF_SIZE);
    strncpy(tmp_buf, data_str, CC_MAX_TEMP_BUF_SIZE - 1);
    token = plat_strtok_r(tmp_buf, ",", &pSave);
    while (token != NULL) {
        data_buf[item_ind] = strtoul(token, NULL, 0);
        item_ind++;
        token = plat_strtok_r(NULL, ",", &pSave);
    }

    free(tmp_buf);
    tmp_buf = NULL;

    return item_ind;
}

const char *get_def_panel_ini_name(void) {
#if (defined CC_COMPILE_IN_PC)
    return "input/panel/panel_default.ini";
#elif (defined CC_COMPILE_IN_ANDROID)
    return "/system/etc/panel_default.ini";
#elif (defined CC_COMPILE_IN_UBOOT)
    return "null";
#endif
}

const char *get_def_panel_pq_db_name(void) {
#if (defined CC_COMPILE_IN_PC)
    return "input/pq/pq_default.db";
#elif (defined CC_COMPILE_IN_ANDROID)
    return "/system/etc/pq_default.db";
#elif (defined CC_COMPILE_IN_UBOOT)
    return "null";
#endif
}

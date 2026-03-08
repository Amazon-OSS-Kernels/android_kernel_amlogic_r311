/*
 * Author:  Shoufu Zhao <shoufu.zhao@amlogic.com>
 */

#include "ini_config.h"

#define LOG_TAG "audio_ini"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_proxy.h"
#include "ini_handler.h"
#include "ini_platform.h"
#include "ini_proj_sum.h"
#include "audio_ini.h"

#define ITEM_DEBUG

#ifdef ITEM_DEBUG
#define ITEM_LOGD(x...) ALOGD(x)
#define ITEM_LOGE(x...) ALOGE(x)
#else
#define ITEM_LOGD(x...)
#define ITEM_LOGE(x...)
#endif

static int handle_integrity_flag(void);

static int handle_regs_data(int max_reg_cnt, struct audio_reg_s *regs, char *section, char *name);
static int handle_one_audio_eq_drc_data(struct audio_eq_drc_reg_s* p_eq_reg, char *section, char *name);

static int handle_audio_eq_data(struct audio_eq_drc_info_s* p_attr);
static int handle_audio_drc_data(struct audio_eq_drc_info_s* p_attr);

static void PrintRegData(int byte_mode, int reg_cnt, struct audio_reg_s* regs);

int handle_audio_eq_drc_ini(char *file_name, struct audio_eq_drc_info_s *p_attr) {
    int i = 0, tmp_ret = 0;

    memset((void *)p_attr, 0, sizeof(struct audio_eq_drc_info_s));

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

    tmp_ret = 0;

    handle_audio_eq_data(p_attr);
    handle_audio_drc_data(p_attr);

    IniParserUninit();

    return 0;
}

int handle_audio_eq_drc_ini_by_id(int proj_id, char *file_name, struct audio_eq_drc_info_s *p_attr) {
    int i = 0, audio_cfg_cnt = 0, default_flag = 0;
    struct project_info_s *pInfoPtr = NULL;

    pInfoPtr = (struct project_info_s *) malloc(sizeof(struct project_info_s) * CC_MAX_SUPPORT_PROJECT_CNT);
    if (pInfoPtr == NULL) {
        ALOGE("%s, malloc project info memory error!!!\n", __FUNCTION__);
        return -1;
    }

    ALOGD("%s, proj_id = %d, project_info_name is (%s)\n", __FUNCTION__, proj_id, file_name);

    default_flag = CC_AUD_EQ_DRC_PATH_MASK;
    audio_cfg_cnt = handle_get_project_all_info(file_name, &default_flag, pInfoPtr);
    ALOGD("%s, audio_cfg_cnt = %d, default flag (0x%08x)\n", __FUNCTION__, audio_cfg_cnt, default_flag);
    if (default_flag & CC_AUD_EQ_DRC_PATH_MASK) {
        audio_cfg_cnt += 1;
    }

    if (audio_cfg_cnt > 0) {
        //refresh current project id's audio ini
        if (proj_id >= 0 && proj_id < audio_cfg_cnt - 1) {
            ALOGD("%s, start refresh current project id(%d)'s audio ini path.\n", __FUNCTION__, proj_id);

            handle_audio_eq_drc_ini(pInfoPtr[proj_id].aud_eq_drc_ini_path, p_attr);
        } else {
            ALOGD("%s, use default audio ini path id(%d).\n", __FUNCTION__, audio_cfg_cnt - 1);

            if (default_flag & CC_AUD_EQ_DRC_PATH_MASK) {
                handle_audio_eq_drc_ini(pInfoPtr[audio_cfg_cnt - 1].aud_eq_drc_ini_path, p_attr);
            } else {
                free(pInfoPtr);
                pInfoPtr = NULL;

                ALOGE("%s, there is no default audio ini path.\n", __FUNCTION__);
                return -1;
            }
        }
    } else {
        free(pInfoPtr);
        pInfoPtr = NULL;

        ALOGE("%s, there is no audio ini path.\n", __FUNCTION__);
        return -1;
    }

    free(pInfoPtr);
    pInfoPtr = NULL;

    return 0;
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

static int handle_audio_eq_data(struct audio_eq_drc_info_s* p_attr) {
    int tmp_ret = 0;
    const char *ini_value = NULL;

    ini_value = IniGetString("eq_param", "eq_enable", "0");
    ITEM_LOGD("%s, eq_enable is (%s)\n", __FUNCTION__, ini_value);
    p_attr->eq_enable = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("eq_param", "eq_name", "0");
    ITEM_LOGD("%s, eq_name is (%s)\n", __FUNCTION__, ini_value);
    strcpy((char *)p_attr->eq_name, ini_value);

    ini_value = IniGetString("eq_param", "eq_byte_mode", "0");
    ITEM_LOGD("%s, eq_byte_mode is (%s)\n", __FUNCTION__, ini_value);
    p_attr->eq_byte_mode = strtoul(ini_value, NULL, 0);

    tmp_ret |= handle_one_audio_eq_drc_data(&p_attr->eq_desk, (char *)"eq_param", (char *)"desk_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(&p_attr->eq_wall, (char *)"eq_param", (char *)"wall_mode_eq_data");

    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_desk.reg_cnt, p_attr->eq_desk.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_wall.reg_cnt, p_attr->eq_wall.regs);

    return tmp_ret;
}

static int handle_audio_drc_data(struct audio_eq_drc_info_s* p_attr) {
    int tmp_ret = 0;
    const char *ini_value = NULL;

    ini_value = IniGetString("drc_param", "drc_enable", "0");
    ITEM_LOGD("%s, drc_enable is (%s)\n", __FUNCTION__, ini_value);
    p_attr->drc_enable = strtoul(ini_value, NULL, 0);

    ini_value = IniGetString("drc_param", "drc_name", "0");
    ITEM_LOGD("%s, drc_name is (%s)\n", __FUNCTION__, ini_value);
    strcpy((char *)p_attr->drc_name, ini_value);

    ini_value = IniGetString("drc_param", "drc_byte_mode", "0");
    ITEM_LOGD("%s, drc_byte_mode is (%s)\n", __FUNCTION__, ini_value);
    p_attr->drc_byte_mode = strtoul(ini_value, NULL, 0);

    tmp_ret |= handle_one_audio_eq_drc_data(&p_attr->drc_ead, (char *)"drc_param", (char *)"drc_ead_table");
    tmp_ret |= handle_one_audio_eq_drc_data(&p_attr->drc_tko, (char *)"drc_param", (char *)"drc_tko_table");

    PrintRegData(p_attr->eq_byte_mode, p_attr->drc_ead.reg_cnt, p_attr->drc_ead.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->drc_tko.reg_cnt, p_attr->drc_tko.regs);

    return tmp_ret;
}

int transBufferData(const char *data_str, unsigned int data_buf[]) {
    int item_ind = 0;
    char *token;
    char *pSave;
    char tmp_buf[4096];

    if (data_str == NULL) {
        return 0;
    }

    memset((void *)tmp_buf, 0, sizeof(tmp_buf));
    strncpy(tmp_buf, data_str, sizeof(tmp_buf) - 1);
    token = strtok_r(tmp_buf, ",", &pSave);
    while (token != NULL) {
        data_buf[item_ind] = strtoul(token, NULL, 0);
        item_ind++;
        token = strtok_r(NULL, ",", &pSave);
    }

    return item_ind;
}

static int handle_regs_data(int max_reg_cnt, struct audio_reg_s *regs, char *section, char *name) {
    int i = 0, j = 0, k = 0, tmp_cnt = 0, tmp_line_cnt = 0;
    const char *ini_value = NULL;
    unsigned int tmp_buf[4096];

    ini_value = IniGetString(section, name, "null");
    //ITEM_LOGD("%s, [%s] %s = %s\n", __FUNCTION__, section, name, ini_value);

    tmp_cnt = transBufferData(ini_value, tmp_buf);
    ITEM_LOGD("%s, reg buffer data cnt = %d\n", __FUNCTION__, tmp_cnt);

    i = 0;
    j = 0;
    while (1) {
        if (j >= max_reg_cnt) {
            break;
        }

        regs[j].len = tmp_buf[i + 0];
        if (regs[j].len == 0) {
            break;
        }

        regs[j].addr = tmp_buf[i + 1];

        for (k = 0; k < (int)regs[j].len; k++) {
            regs[j].data[k] = tmp_buf[i + 2 + k];
        }
        i += regs[j].len + 2;
        j += 1;
    }

    return j;
}

static int handle_one_audio_eq_drc_data(struct audio_eq_drc_reg_s* p_reg, char *section, char *name) {
    int tmp_ret = 0;

    memset((void *)p_reg, 0, sizeof(struct audio_eq_drc_reg_s));

    p_reg->reg_cnt = CC_AUDIO_EQ_REG_CNT_MAX;

    tmp_ret = handle_regs_data(p_reg->reg_cnt, p_reg->regs, section, name);
    if (tmp_ret <= 0 || tmp_ret > p_reg->reg_cnt) {
        memset((void *)p_reg, 0, sizeof(struct audio_eq_drc_reg_s));
        return -1;
    }

    p_reg->reg_cnt = tmp_ret;

    return 0;
}

static int ExportDataAsBin(const char *fname_str, void *data, int data_len) {
    int i = 0, total_len = 0;
    int fd = -1;

    if (fname_str == NULL || data == NULL || data_len == 0) {
        return -1;
    }

    fd = open(fname_str, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        ALOGE("%s, Open %s ERROR(%s)!!\n", __FUNCTION__, fname_str, strerror(errno));
        return -1;
    }

    write(fd, data, data_len);

    close(fd);
    fd = -1;

    return 0;
}

static void PrintRegData(int byte_mode, int reg_cnt, struct audio_reg_s* regs) {
    int i = 0, j = 0, tmp_len = 0;
    char tmp_buf[1024] = {'\0'};

    ITEM_LOGD("%s, reg_cnt = %d\n", __FUNCTION__, reg_cnt);
    memset(tmp_buf, 0, 1024);
    for (i = 0; i < reg_cnt; i++) {
        tmp_len = strlen(tmp_buf);
        sprintf((char *)tmp_buf + tmp_len, "%d, ", regs[i].len);

        tmp_len = strlen(tmp_buf);
        if (byte_mode == 4) {
            sprintf((char *)tmp_buf + tmp_len, "0x%08X, ", regs[i].addr);
        } else {
            sprintf((char *)tmp_buf + tmp_len, "0x%02X, ", regs[i].addr);
        }

        for (j = 0; j < (int)regs[i].len; j++) {
            tmp_len = strlen(tmp_buf);
            if (byte_mode == 4) {
                sprintf((char *)tmp_buf + tmp_len, "0x%08X, ", regs[i].data[j]);
            } else {
                sprintf((char *)tmp_buf + tmp_len, "0x%02X, ", regs[i].data[j]);
            }
        }

        ITEM_LOGD("%s", tmp_buf);

        memset(tmp_buf, 0, 1024);
    }

    ITEM_LOGD("\n\n");
}

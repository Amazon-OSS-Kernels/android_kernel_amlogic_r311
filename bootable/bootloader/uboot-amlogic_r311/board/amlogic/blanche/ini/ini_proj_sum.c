/*
 * Author:  Shoufu Zhao <shoufu.zhao@amlogic.com>
 */

#include "ini_config.h"

#define LOG_TAG "ini_proj_sum"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_proxy.h"
#include "ini_proj_sum.h"

//#define ITEM_DEBUG

#if (defined ITEM_DEBUG)
    #define ITEM_LOGD(x...) ALOGD(x)
    #define ITEM_LOGE(x...) ALOGE(x)
#else
    #define ITEM_LOGD(x...)
    #define ITEM_LOGE(x...)
#endif

int handle_get_project_all_info(char *file_name, int *default_flag, struct project_info_s* pInfoPtr) {
    int i = 0, total_cnt = 0, mode_val = 0;
    char tmp_buf[64] = {0};
    const char *ini_value = NULL;

    IniParserInit();

    if (IniParseFile(file_name) < 0) {
        ALOGE("%s, ini load file error!\n", __FUNCTION__);

        IniParserUninit();
        return -1;
    }

    mode_val = *default_flag;

    total_cnt = 0;
    while(1) {
        sprintf(tmp_buf, "%d", total_cnt);

        ini_value = IniGetString(tmp_buf, "PANELINI_PATH", "null");
        ITEM_LOGD("%s, %s's PANELINI_PATH is (%s)\n", __FUNCTION__, tmp_buf, ini_value);
        if (strcmp(ini_value, "null") == 0) {
            if (mode_val & CC_PANEL_INI_PATH_MASK) {
                break;
            }
        } else {
            strcpy(pInfoPtr[total_cnt].panel_ini_path, ini_value);
        }

        ini_value = IniGetString(tmp_buf, "PQINI_PATH", "null");
        ITEM_LOGD("%s, %s's PQINI_PATH is (%s)\n", __FUNCTION__, tmp_buf, ini_value);
        if (strcmp(ini_value, "null") == 0) {
            if (mode_val & CC_PANEL_PQ_PATH_MASK) {
                break;
            }
        } else {
            strcpy(pInfoPtr[total_cnt].panel_pq_path, ini_value);
        }

        ini_value = IniGetString(tmp_buf, "HDRINI_PATH", "null");
        ITEM_LOGD("%s, %s's HDRINI_PATH is (%s)\n", __FUNCTION__, tmp_buf, ini_value);
        if (strcmp(ini_value, "null") == 0) {
            if (mode_val & CC_HDR_PATH_MASK) {
                break;
            }
        } else {
            strcpy(pInfoPtr[total_cnt].hdr_ini_path, ini_value);
        }

        ini_value = IniGetString(tmp_buf, "AUDIO_EQ_DRC_PATH", "null");
        ITEM_LOGD("%s, %s's AUDIO_EQ_DRC_PATH is (%s)\n", __FUNCTION__, tmp_buf, ini_value);
        if (strcmp(ini_value, "null") == 0) {
            if (mode_val & CC_AUD_EQ_DRC_PATH_MASK) {
                break;
            }
        } else {
            strcpy(pInfoPtr[total_cnt].aud_eq_drc_ini_path, ini_value);
        }

        ini_value = IniGetString(tmp_buf, "AUDIO_CURVE_PATH", "null");
        ITEM_LOGD("%s, %s's AUDIO_CURVE_PATH is (%s)\n", __FUNCTION__, tmp_buf, ini_value);
        if (strcmp(ini_value, "null") == 0) {
            if (mode_val & CC_AUD_CURVE_PATH_MASK) {
                break;
            }
        } else {
            strcpy(pInfoPtr[total_cnt].aud_curve_ini_path, ini_value);
        }

        total_cnt += 1;
    }

    *default_flag = 0;

    if (mode_val & CC_PANEL_INI_PATH_MASK) {
        ini_value = IniGetString("default", "PANELINI_PATH", "null");
        ITEM_LOGD("%s, %s's PANELINI_PATH is (%s)\n", __FUNCTION__, "default", ini_value);
        if (strcmp(ini_value, "null")) {
            *default_flag |= CC_PANEL_INI_PATH_MASK;
            strcpy(pInfoPtr[total_cnt].panel_ini_path, ini_value);
            ITEM_LOGD("%s, default's PANELINI_PATH is (%s)\n", __FUNCTION__, pInfoPtr[total_cnt].panel_ini_path);
        }
    }

    if (mode_val & CC_PANEL_PQ_PATH_MASK) {
        ini_value = IniGetString("default", "PQINI_PATH", "null");
        ITEM_LOGD("%s, %s's PQINI_PATH is (%s)\n", __FUNCTION__, "default", ini_value);
        if (strcmp(ini_value, "null")) {
            *default_flag |= CC_PANEL_PQ_PATH_MASK;
            strcpy(pInfoPtr[total_cnt].panel_pq_path, ini_value);
            ITEM_LOGD("%s, default's PQINI_PATH is (%s)\n", __FUNCTION__, pInfoPtr[total_cnt].panel_pq_path);
        }
    }

    if (mode_val & CC_HDR_PATH_MASK) {
        ini_value = IniGetString("default", "HDRINI_PATH", "null");
        ITEM_LOGD("%s, %s's HDRINI_PATH is (%s)\n", __FUNCTION__, "default", ini_value);
        if (strcmp(ini_value, "null")) {
            *default_flag |= CC_HDR_PATH_MASK;
            strcpy(pInfoPtr[total_cnt].hdr_ini_path, ini_value);
            ITEM_LOGD("%s, default's HDRINI_PATH is (%s)\n", __FUNCTION__, pInfoPtr[total_cnt].hdr_ini_path);
        }
    }

    if (mode_val & CC_AUD_EQ_DRC_PATH_MASK) {
        ini_value = IniGetString("default", "AUDIO_EQ_DRC_PATH", "null");
        ITEM_LOGD("%s, %s's AUDIO_EQ_DRC_PATH is (%s)\n", __FUNCTION__, "default", ini_value);
        if (strcmp(ini_value, "null")) {
            *default_flag |= CC_AUD_EQ_DRC_PATH_MASK;
            strcpy(pInfoPtr[total_cnt].aud_eq_drc_ini_path, ini_value);
            ITEM_LOGD("%s, default's AUDIO_EQ_DRC_PATH is (%s)\n", __FUNCTION__, pInfoPtr[total_cnt].aud_eq_drc_ini_path);
        }
    }

    if (mode_val & CC_AUD_CURVE_PATH_MASK) {
        ini_value = IniGetString("default", "AUDIO_CURVE_PATH", "null");
        ITEM_LOGD("%s, %s's AUDIO_CURVE_PATH is (%s)\n", __FUNCTION__, "default", ini_value);
        if (strcmp(ini_value, "null")) {
            *default_flag |= CC_AUD_CURVE_PATH_MASK;
            strcpy(pInfoPtr[total_cnt].aud_curve_ini_path, ini_value);
            ITEM_LOGD("%s, default's AUDIO_CURVE_PATH is (%s)\n", __FUNCTION__, pInfoPtr[total_cnt].aud_curve_ini_path);
        }
    }

    for(i = 0; i < total_cnt; i++) {
        ITEM_LOGD("%s, %d's PANELINI_PATH is (%s)\n", __FUNCTION__, i, pInfoPtr[i].panel_ini_path);
        ITEM_LOGD("%s, %d's PQINI_PATH is (%s)\n", __FUNCTION__, i, pInfoPtr[i].panel_pq_path);
        ITEM_LOGD("%s, %d's HDRINI_PATH is (%s)\n", __FUNCTION__, i, pInfoPtr[i].hdr_ini_path);
    }

    IniParserUninit();

    return total_cnt;
}

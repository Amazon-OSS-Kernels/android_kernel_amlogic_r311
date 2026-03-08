#include "ini_config.h"

#define LOG_TAG "ini_powerkey"
#define LOG_NDEBUG 0

#include "ini_log.h"
#include "ini_proxy.h"
#include "ini_powerkey.h"
#include "ini_platform.h"

#define CC_MAX_DATA_SIZE                    (0x400000)
#define POWERKEY_IR_INI_PATH                "/factory/customer/uboot_ir_power.ini"
#define POWERKEY_KEYPAD_INI_PATH            "/factory/customer/uboot_keypad_power.ini"

static int handle_ir_parse(void)
{
    const char *ini_value = NULL;
    const char *ini_value4 = NULL;
    char tmp_buf[64] = {0};
    char *ir_power_key = NULL;
    char *ir_power_key4 = NULL;

    ini_value = IniGetString(tmp_buf, "ir_power_key", "null");
    printf("%s, ini ir_power_key is (%s)\n", __FUNCTION__, ini_value);
    if (!ini_value) {
        printf("ini ir_power_key is null, keep env val!\n");
        return 0;
    }

    ir_power_key = getenv("ir_power_key3");
    if (!ir_power_key)
    {
        printf("not set ir ir_power_key,or try ir_power_key default -a\n");
        return -1;
    }

    printf(" getenv ir_power_key = %s\n", ir_power_key);

    if(strcmp(ir_power_key, ini_value) != 0) {
        sprintf(tmp_buf, "setenv ir_power_key3 %s", ini_value);
        run_command(tmp_buf, 0);
        run_command("save", 0);
    }

    ini_value4 = IniGetString(tmp_buf, "ir_power_key4", "null");
    printf("%s, ini ir_power_key4 is (%s)\n", __FUNCTION__, ini_value4);
    if (!ini_value4) {
        printf("ini ir_power_key4 is null, keep env val!\n");
        return 0;
    }

    ir_power_key4 = getenv("ir_power_key4");
    if (!ir_power_key4)
    {
        printf("not set ir ir_power_key4,or try ir_power_key4 default -a\n");
        return -1;
    }

    printf(" getenv ir_power_key4 = %s\n", ir_power_key4);

    if(strcmp(ir_power_key4, ini_value4) != 0) {
        sprintf(tmp_buf, "setenv ir_power_key4 %s", ini_value4);
        run_command(tmp_buf, 0);
        run_command("save", 0);
    }

    return 0;
}

static int handle_keypad_parse(void)
{
    const char *ini_value = NULL;
    char tmp_buf[64] = {0};
    char *adc_ch_power_key = NULL;
    ini_value = IniGetString(tmp_buf, "adc_ch_power_key", "null");
    printf("%s, ini adc_ch_power_key is (%s)\n", __FUNCTION__, ini_value);

    if (!ini_value) {
        printf("ini adc_ch_power_key is null, keep env val!\n");
        return 0;
    }
    adc_ch_power_key = getenv("adc_ch_power_key");
    if (!adc_ch_power_key)
    {
        printf("not set ir adc_ch_power_key,or try adc_ch_power_key default -a\n");
        return -1;
    }
    printf(" getenv adc_ch_power_key = %s\n", adc_ch_power_key);
    if(strcmp(adc_ch_power_key, ini_value) != 0) {
        sprintf(tmp_buf, "setenv adc_ch_power_key %s", ini_value);
        run_command(tmp_buf, 0);
        run_command("save", 0);
    }
    return 0;
}
int parse_ir_powerkey_ini(char *file_name)
{
    IniParserInit();

    if (IniParseFile(file_name) < 0)
    {
        ALOGE("%s, ini load file error!\n", __FUNCTION__);
        IniParserUninit();
        return -1;
    }

    // handle integrity flag
    if (handle_ir_parse() < 0)
    {
        IniParserUninit();
        return -1;
    }

    IniParserUninit();

    return 0;
}

int parse_keypad_powerkey_ini(char *file_name)
{
    IniParserInit();

    if (IniParseFile(file_name) < 0)
    {
        ALOGE("%s, ini load file error!\n", __FUNCTION__);
        IniParserUninit();
        return -1;
    }

    // handle integrity flag
    if (handle_keypad_parse() < 0)
    {
        IniParserUninit();
        return -1;
    }

    IniParserUninit();

    return 0;
}

int parse_powerkey_ini(void)
{
    int ret = 0;
    char *ir_file_name = POWERKEY_IR_INI_PATH;
    char *keypad_file_name = POWERKEY_KEYPAD_INI_PATH;

    if (!iniIsFileExist(keypad_file_name))
    {
        printf("%s, file name \"%s\" not exist.\n", __FUNCTION__, keypad_file_name);
        return -1;
    }

    if (!iniIsFileExist(ir_file_name))
    {
        printf("%s, file name \"%s\" not exist.\n", __FUNCTION__, keypad_file_name);
        return -1;
    }

    if(parse_keypad_powerkey_ini(keypad_file_name) < 0) {
        ret = -1;
        printf("parse_keypad_powerkey_ini failed!\n");
    }

    if(parse_ir_powerkey_ini(ir_file_name) < 0) {
        ret = -1;
        printf("parse_ir_powerkey_ini failed!\n");
    }
    return ret;
}



int Ini_check_powerkey()
{
    int ret = -1;
    unsigned char *tmp_buf = NULL;
    unsigned char *parse_buf = NULL;

    tmp_buf = (unsigned char *) malloc(CC_MAX_DATA_SIZE);
    if (tmp_buf == NULL)
    {
        printf("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    parse_buf = (unsigned char *) malloc(CC_MAX_DATA_SIZE);
    if (parse_buf == NULL)
    {
        free(tmp_buf);
        tmp_buf = NULL;
        printf("%s, malloc buffer memory error!!!\n", __FUNCTION__);
        return -1;
    }

    if (parse_powerkey_ini() < 0)
    {
            ret = -1;
            goto out;
    }

out:
    free(tmp_buf);
    tmp_buf = NULL;
    free(parse_buf);
    parse_buf = NULL;

    return ret;
}


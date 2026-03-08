/*
 * U-boot - read and store param on logo partition
 *
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * Licensed under the GPL-2 or later.
 */

#include <stdlib.h>
#include <config.h>
#include <common.h>
#include <amlogic/logo_param.h>
#include <amlogic/storage_if.h>


/*
* Total logo partition is 4096K. The last 64K is used for store the param
* Spaces used by logo partition are as below:
* 0   - 4032K     For original logo
* 4032K  - 4048K  Used by HDMI display param (16K)
* 4048K  - 4096K  Used by other param (48K)
* Note that these offsets are admitted by bootloader,recovery and uncrypt, so they
* are not configurable without changing all of them.
*/

#define MAX_UBOOTLOGOPARAM_KEY_LENGTH       256
#define MAX_UBOOTLOGOPARAM_VALUE_LENGTH     1024

#define LOGOPARAM_OFFSET_IN_LOGO            0x3F4000 // 4048 * 1024;
#define MAX_LOGOPARAM_SIZE                  0xC000 // 48 * 1024;
#define PROFIX_LOGO_PARAM_VAR "logoparam.var."

typedef struct uboot_logoparam_image {
    uint32_t crc;
    char data[];
} uboot_logoparam_image_t;

typedef struct uboot_logoparam {
    void *image;
    uint32_t *crc;
    char *data;
} uboot_logoparam_t;

typedef struct uboot_logoparam_attribute {
    struct uboot_logoparam_attribute *next;
    char key[MAX_UBOOTLOGOPARAM_KEY_LENGTH];
    char value[MAX_UBOOTLOGOPARAM_VALUE_LENGTH];
} uboot_logoparam_attribute_t;

bool logoparam_initflag = false;
bool logoparam_eraseflag = false;
uboot_logoparam_t logoparamdata;
int logoparamdatasize;
uboot_logoparam_attribute_t logoparamattrheader;



#if 0
static void logoparam_printData(void) { //test func: need to be removed
    char data[2001];
    memset(data, 0, 2001);
    memcpy(data, logoparamdata.data, 2000);

    int i=0;
    int j=0;
    for(;i<2000; i++) {
        if(data[i] == '\0') {
            j = i+1;
            if(data[j] == '\0') {
                break;
            }

            data[i] = ' ';
        }
    }

    printf("Line %d: lji data: %s\n", __LINE__, data);
}
#endif



static bool logoparam_checkKey(const char * prop_name) {
    if (!prop_name || !(*prop_name)) {
        printf("[logoparam] NULL Key\n");
        return false;
    }

    if (strncmp(prop_name, PROFIX_LOGO_PARAM_VAR, strlen(PROFIX_LOGO_PARAM_VAR)) == 0
        && strlen(prop_name) > strlen(PROFIX_LOGO_PARAM_VAR) )
        return true;

    printf("[logoparam] Invaild Key\n");
    return false;
}

static void logoparam_parseattr(void) {
    //printf("func: %s Entrance\n",__func__);
    char *proc = logoparamdata.data;
    char *nextProc;
    uboot_logoparam_attribute_t *attr = &logoparamattrheader;

    do {
        nextProc = proc + strlen(proc) + sizeof(char);
        char *key = strchr(proc, (int)'=');
        if (key != NULL) {
            *key=0;
            strcpy(attr->key, proc);
            strcpy(attr->value, key + sizeof(char));
        } else {
            printf("Error: %s: error need '=' skip this value\n", __func__);
            break;
        }

        if (!(*nextProc)) {
            break;
        }
        proc = nextProc;

        attr->next = (uboot_logoparam_attribute_t *)malloc(sizeof(uboot_logoparam_attribute_t));
        if (attr->next == NULL) {
            printf("Error: %s: parse attribute malloc error\n", __func__);
            break;
        }
        memset(attr->next, 0, sizeof(uboot_logoparam_attribute_t));
        attr = attr->next;
    }while(1);
}

static int read_logoparam_struct(void) {
    //printf("func: %s Entrance\n",__func__);
    unsigned char* imageaddr = (unsigned char *)malloc(MAX_LOGOPARAM_SIZE);
    if(imageaddr == NULL){
        printf("Error: %s: Not enough memory for imageaddr\n", __func__);
        return -1;
    }

    logoparamdata.image = imageaddr;
    uboot_logoparam_image_t *ptrimage = (uboot_logoparam_image_t *)imageaddr;
    logoparamdata.crc = &(ptrimage->crc);
    logoparamdata.data = ptrimage->data;

    if(store_read_ops((unsigned char*)"logo", (unsigned char*)(imageaddr),
        LOGOPARAM_OFFSET_IN_LOGO, MAX_LOGOPARAM_SIZE)) {
        printf("Error: %s: store_read_ops failed \n",__func__);
        return -1;
    }
    //logoparam_printData();//test func: need to be removed

    if(*(logoparamdata.crc) == 0) {
        /* At this moment,
         * thers is no data which was stored on logo partiton,
         * So we do nothing here
         */
        return 0;
    }

    uint32_t crcCalc = crc32(0, (uint8_t *)logoparamdata.data, logoparamdatasize);
    if(crcCalc != *(logoparamdata.crc)) {
        printf("Error: %s: CRC Check SYS_LOGE save_crc=%08x, crcCalc = %08x \n\n",
           __func__, *(logoparamdata.crc), crcCalc);
        return -2;
    }

    logoparam_parseattr();

    return 0;
}

static int logoparam_init(void) {
    //printf("func: %s Entrance\n",__func__);
    logoparam_initflag = false;
    memset(&logoparamdata, 0, sizeof(logoparamdata));
    logoparamdatasize = MAX_LOGOPARAM_SIZE - sizeof(uint32_t);
    memset(&logoparamattrheader, 0, sizeof(logoparamattrheader));

    if(read_logoparam_struct() < 0)
        return -1;

    logoparam_initflag = true;
    return 0;
}

static char* logoparam_getvalue(const char* key){
    //printf("func: %s Entrance\n",__func__);
    uboot_logoparam_attribute_t *attr = &logoparamattrheader;
    while (attr) {
        if (!strcmp(key, attr->key)) {
            // printf("Info: %s: get succeed: key: %s, value: %s\n", __func__,key, attr->value);
            return attr->value;
        }
        attr = attr->next;
    }
    return NULL;
}

static int logoparam_setattribute(const char * key,  const char * value, bool createNew) {
    //printf("func: %s Entrance\n",__func__);
    uboot_logoparam_attribute_t *attr = &logoparamattrheader;
    uboot_logoparam_attribute_t *last = attr;
    while (attr) {
        if (!strcmp("", attr->key)) {
            strcpy(attr->key, key);
            strcpy(attr->value, value);
            return 1;
        }
        if (!strcmp(key, attr->key)) {
            strcpy(attr->value, value);
            return 2;
        }
        last = attr;
        attr = attr->next;
    }

    if (createNew) {
        printf("Info: %s: not find key: %s, create it\n", __func__,key);
        attr = (uboot_logoparam_attribute_t *)malloc(sizeof(uboot_logoparam_attribute_t));
        if (attr == NULL) {
            printf("Error: %s: Not enough memory for param\n", __func__);
            return -1;
        }
        last->next = attr;
        memset(attr, 0, sizeof(uboot_logoparam_attribute_t));
        strcpy(attr->key, key);
        strcpy(attr->value, value);
        return 1;
    }
    return 0;
}


/*  attribute revert to sava data*/
static int logoparam_formatattribute(void) {
//    printf("func: %s Entrance\n",__func__);
    uboot_logoparam_attribute_t *attr = &logoparamattrheader;
    char *data = logoparamdata.data;
    memset(logoparamdata.data, 0, logoparamdatasize);
    int size = 0;

    if(logoparam_eraseflag)
        return 0;

    do {
        size = data - logoparamdata.data;
        if(size >= logoparamdatasize) {
            printf("Error: %s: there is not enough space on /dev/block/logo to store a new param\n", __func__);
            return -1;
        }

        int len = sprintf(data, "%s=%s", attr->key, attr->value);
        if (len < (int)(sizeof(char)*3)) {
            printf("Error: %s: Invalid env data key:%s, value:%s\n", __func__,attr->key, attr->value);
        }
        else
            data += len + sizeof(char);

        attr = attr->next;
    } while (attr);

    return 0;
}

//save value to storage flash
static int write_logoparam_struct(void) {
    //printf("func: %s Entrance\n",__func__);
    if(logoparam_formatattribute() < 0)
        return -1;

    *(logoparamdata.crc) = crc32(0, (uint8_t *)logoparamdata.data, logoparamdatasize);

    if(logoparam_eraseflag)
        *(logoparamdata.crc) = 0;

    if(store_write_ops((unsigned char*)"logo", (unsigned char*)(logoparamdata.image),
        LOGOPARAM_OFFSET_IN_LOGO, MAX_LOGOPARAM_SIZE) < 0) {
        printf("Error: %s: store write failed\n", __func__);
        return -1;
    }

    return 0;
}

#if 0
static void logoparam_print_all(void) { //test func: need to be removed
    if (logoparam_initflag == false) {
        if (logoparam_init() < 0) {
            printf("Logo param init error\n");
            return ;
        }
    }


    //printf("func: %s Entrance\n",__func__);
    uboot_logoparam_attribute_t *attr = &logoparamattrheader;
    while (attr) {
        printf("   key: %s, value: %s\n", attr->key, attr->value);
        attr = attr->next;
    }
    return ;
}
#endif




static char* get_logoparam_value(const char* key) {
    if (!logoparam_checkKey(key)) {
        return NULL;
    }

    if (logoparam_initflag == false) {
        if(logoparam_init() < 0)
            return NULL;
    }

    return logoparam_getvalue(key);
}

static int update_logoparam_value(const char* key, const char* value) {
    //printf("func: %s Entrance\n",__func__);
    if (!logoparam_checkKey(key)) {
        return -1;
    }

    if (logoparam_initflag == false) {
        if(logoparam_init() < 0){
            printf("Error: update_logoparam_value logoparam_init failed\n");
            return -1;
        }
    }

    const char *logoparamValue = logoparam_getvalue(key);
    if (!logoparamValue)
        logoparamValue = "";

    if (!strcmp(value, logoparamValue))
        return 0;

    if (logoparam_setattribute(key, value, true) < 0)
        return -1;

    return write_logoparam_struct();
}

static int update_logoparam_ValueByarray(const char** name, const char** value, int arraysize) {
    //printf("func: %s Entrance\n",__func__);
    if (logoparam_initflag == false) {
        if(logoparam_init() < 0){
            printf("Error: update_logoparam_ValueByarray logoparam_init failed\n");
            return -1;
        }
    }

    int index;
    bool isNeedUpdate = false;
    for (index = 0; index < arraysize; index++) {
        if(!logoparam_checkKey(name[index]))
            return -1;

        const char *logoparamValue = logoparam_getvalue(name[index]);
        if (!logoparamValue)
            logoparamValue = "";
         if (strcmp(value[index], logoparamValue)){
            isNeedUpdate = true;
            break;
         }
    }

    if(!isNeedUpdate)
        return 0;

    for (index = 0; index < arraysize; index++) {
        if(logoparam_setattribute(name[index], value[index], true)<0)
            return -1;
    }

    return write_logoparam_struct();
}

void erase_logoparam_data(void)
{
    logoparam_eraseflag = true;
    write_logoparam_struct();
    uboot_logoparam_t logoparamdata;
    if(logoparamdata.image){
        free(logoparamdata.image);
        logoparamdata.image = NULL;
        logoparamdata.crc = NULL;
        logoparamdata.data = NULL;
    }

    uboot_logoparam_attribute_t * pAttr = logoparamattrheader.next;
    memset(&logoparamattrheader, 0, sizeof(uboot_logoparam_attribute_t));
    uboot_logoparam_attribute_t * pTmp = NULL;
    while (pAttr) {
        pTmp = pAttr;
        pAttr = pAttr->next;
        free(pTmp);
    }

    logoparam_initflag = false;
    logoparam_eraseflag = false;
}




/*
function:
set key of value to logo partition.
param:
if is_set_env is true, will call setenv function.
if is_env_default is true, when uese the env default value.
*/
int set_logoparam_value_ex(const char* key, const char* value, bool is_set_env) {
    char * env_key = NULL;

    // get env key
    env_key = strrchr(key, '.');
    if (env_key)
        env_key++;

    // save the value to logo
    if (update_logoparam_value(key, value) != 0) {
        printf("save %s=%s to logo error\n", key, value);
        return 0;
    }

    // set the value to env memory.
    if (is_set_env && (env_key != NULL)) {
        setenv(env_key, value);
    }

    return 1;
}


/*
function:
get key of value from logo partition.
param:
if is_set_env is true, will call setenv function.
if is_env_default is true, when uese the env default value.
*/
char* get_logoparam_value_ex(const char* key, bool is_set_env, bool is_env_default) {
    char * val = NULL;
    char * env_key = NULL;

    // get env key
    env_key = strrchr(key, '.');
    if (env_key)
        env_key++;

    val = get_logoparam_value(key);
    if (val) {
        printf("%s: %s=%s\n", __FUNCTION__, key, val);
        // set the value to env memory.
        if (is_set_env && (env_key != NULL) && (val[0] != 0)) {
            setenv(env_key, val);
        }
    } else {
        // uses the env default to set logo value.
        if ((is_env_default) && (env_key != NULL)) {
            val = getenv(env_key);
            if (val == NULL) {
                return NULL;
            }
            printf("get_logoparam_value_ex: %s is NULL, use default:%s\n", key, val);
            update_logoparam_value(key, val);
        }
    }

    return val;
}


// UBOOT command line test code
#if 0
static int do_logo_read(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    if (argc <= 1) {
        logoparam_print_all();
    } else {
        char * key_value = get_logoparam_value(argv[1]);
        printf(" %s=%s\n", argv[1], key_value);
    }

	return 1;
}
static int do_logo_write(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    if (argc < 3) {
        return cmd_usage(cmdtp);
    }

    int ret = update_logoparam_value(argv[1], argv[2]);
    if (ret == 0) {
        printf("write %s=%s OK!\n", argv[1], argv[2]);
    } else {
        printf("write %s=%s FAILED!\n", argv[1], argv[2]);
        cmd_usage(cmdtp);
    }

	return 1;
}

static int do_logo_erase(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    if (argc <= 1) {
        erase_logoparam_data();
    }

	return 1;
}

static cmd_tbl_t cmd_logo_sub[] = {
	U_BOOT_CMD_MKENT(read, 2, 1, do_logo_read, "", ""),
	U_BOOT_CMD_MKENT(write, 3, 1, do_logo_write, "", ""),
	U_BOOT_CMD_MKENT(erase, 3, 1, do_logo_erase, "", "")
};

static int do_logo_param(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;
#if 0
    int i = 0;
    for (i=0; i<argc; i++){
        printf(" argv[%d]=%s\n", i, argv[i]);
    }
#endif

	c = find_cmd_tbl(argv[0], &cmd_logo_sub[0], ARRAY_SIZE(cmd_logo_sub));
	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(lparam, CONFIG_SYS_MAXARGS, 0, do_logo_param,
	   "Logo param read write",
	"lparam read [param name]\n"
	"    read logo param\n"
	"lparam write [param name]\n"
	"    write logo param\n"
	"lparam erase [param name]\n"
	"    erase logo param\n"
);
#endif







static int do_logo_getlogo(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    if (argc <= 1) {
        printf("lparam get key\n");
        return 0;
    }

    char * key_value = get_logoparam_value_ex(argv[1], true, false);
    // printf(" %s=%s\n", argv[1], key_value);
	return 1;
}

static int do_logo_setlogo(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    if (argc < 3) {
        printf("setlogo key value\n");
        return 0;
    }
    set_logoparam_value_ex(argv[1], argv[2], true);
	return 1;
}



U_BOOT_CMD(getlogo, 2, 0, do_logo_getlogo,
	   "Logo param get and call setenv\n",
	"getlogo [param name]\n"
);


U_BOOT_CMD(setlogo, 3, 0, do_logo_setlogo,
	   "Logo param write and call setenv\n",
	"setlogo key value\n"
);


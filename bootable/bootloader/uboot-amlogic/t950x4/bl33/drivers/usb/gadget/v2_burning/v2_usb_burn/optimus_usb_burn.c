/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
* *
This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* *
This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
* *
You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
* *
Description:
*/

#include "../v2_sdc_burn/optimus_sdc_burn_i.h"
#include "../v2_sdc_burn/optimus_led.h"

extern int optimus_burn_with_cfg_file(const char* cfgFile);
#ifdef USB_UPGRADE_IN_ONE_FILE
extern bool amzn_target_is_lockdown();

#ifdef CONFIG_IDME
#include <idme.h>
#endif
#endif

// added by scy
int optimus_burn_package_in_usb(const char* sdc_cfg_file)
{
    int rcode = 0;

    DWN_MSG("usb start\n");
    rcode = run_command("usb start", 0);
    if (rcode) {
        DWN_ERR("Fail in init usb host, Does usb host not plugged in?\n");
        return __LINE__;
    }

#if 1//this asserted by 'run update' and 'aml_check_is_ready_for_sdc_produce'
    rcode = do_fat_get_fileSz(sdc_cfg_file);
    if (!rcode) {
        DWN_ERR("The [%s] not exist in udisk\n", sdc_cfg_file);
        return __LINE__;
    }
#endif//#if 0

    rcode = optimus_device_probe("usb", "0");
    if (rcode) {
        DWN_ERR("Fail to detect device usb 0\n");
        return __LINE__;
    }

    rcode = optimus_burn_with_cfg_file(sdc_cfg_file);

    return rcode;
}


// added by scy
int do_usb_burn(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

    int rcode = 0;
    const char* sdc_cfg_file = argv[1];

    #ifdef USB_UPGRADE_IN_ONE_FILE
    if(amzn_target_is_lockdown()){
        printf("target is lockdonw , please unlock!\n");
        return __LINE__;
    }
    #endif

    setenv("usb_update","1");

    if (argc < 2 ) {
        cmd_usage(cmdtp);
        return __LINE__;
    }

    optimus_work_mode_set(OPTIMUS_WORK_MODE_SDC_UPDATE);
    show_logo_to_report_burning();//indicate enter flow of burning! when 'run update'
    if (optimus_led_open(LED_TYPE_PWM)) {
        DWN_ERR("Fail to open led for burn\n");
        return __LINE__;
    }
    optimus_led_show_in_process_of_burning();

    #ifdef USB_UPGRADE_IN_ONE_FILE
    run_command("osd clear", 0);
    #endif

    rcode = optimus_burn_package_in_usb(sdc_cfg_file);

    return rcode;
}

int do_usb_upgrade_fos( )
{
    int rcode = 0;

#ifdef USB_UPGRADE_IN_ONE_FILE
    char sdc_cfg_file[100] = {0};
    char oem_data[24] = {0};
    char* argv[2] = {0, 0};
    char* ammo_title = "ammo_var=";
    char* ammo_var;

    idme_get_var_external("oem_data", oem_data, sizeof(oem_data));
    ammo_var = strstr(oem_data, ammo_title) + strlen(ammo_title);
    if (ammo_var == NULL) {
        DWN_ERR("ammo_var does not exist, upgrade fail!\n");
        return rcode;
    }

    strcpy(sdc_cfg_file, ammo_var);
    strcpy(sdc_cfg_file + strlen(ammo_var), "_usb_burn_package.img");
    printf("ready to flash %s\n", sdc_cfg_file);

    argv[1] = sdc_cfg_file;
    rcode = do_usb_burn(NULL, 0, 2, argv);
#endif

    return rcode;
}

// added by scy
U_BOOT_CMD(
   usb_burn,      //command name
   5,               //maxargs
   0,               //repeatable
   do_usb_burn,   //command function
   "Burning with amlogic format package in usb ",           //description
   "argv: [sdc_burn_cfg_file]\n"//usage
   "    -aml_sdc_burn.ini is usually used configure file\n"
);

U_BOOT_CMD(
   usb_upgrade_fos,      //command name
   1,               //maxargs
   0,               //repeatable
   do_usb_upgrade_fos,   //command function
   "Burning with amlogic format package in usb automastic ",           //description
   ""//usage
   "\n"
);

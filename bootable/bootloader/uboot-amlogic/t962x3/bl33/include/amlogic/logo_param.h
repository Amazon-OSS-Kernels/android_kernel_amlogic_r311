/*
 * U-boot - read and store param on logo partition
 *
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __LOGO_PARAM_H__
#define __LOGO_PARAM_H__

/*
char* get_logoparam_value(const char* key);
int update_logoparam_value(const char* key, const char* value);
int update_logoparam_ValueByarray(const char** name, const char** value, int arraysize);
*/
void erase_logoparam_data(void);


int set_logoparam_value_ex(const char* key, const char* value, bool is_set_env);
char* get_logoparam_value_ex(const char* key, bool is_set_env, bool is_env_default);
#endif


/* Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved. */
#ifndef AMZN_LOCKDOWN
#define AMZN_LOCKDOWN

#include <stdbool.h>
/**
 * Issued when we enter interactive prompt so blacklisted
 * commands should be blocked
 */
void amzn_block_commands(void);

/**
 * Checks if the command can be allowed to run
 */
bool amzn_is_command_blocked(const char *cmd);

int query_efuse_status(const char *item);

#endif

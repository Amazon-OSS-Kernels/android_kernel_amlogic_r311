/*
Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*/
#include <string.h>
#include <stdio.h>
#include "ufbl_debug.h"
#include "idme.h"

#define MAX_LINE_LENGTH  1024
#define CMD_TAG  "idme"
#define PROMPT_STR  ">>> "

static int getcmd(char *cmd);
static int idme_process_cmd(const char *cmd);
/**
* @brief primary function to read and process idme command
*
*/
int itt_idme_process(void)
{
	char cmd[MAX_LINE_LENGTH];
	int ret = 0;

	while (getcmd(cmd)) {
		printf("%s", PROMPT_STR);
		printf("Got command : %s\n", cmd);
		ret = idme_process_cmd(cmd);
	}

	return ret;
}
/**
*
*@brief This function presents a prompt and reads command string
*
*@param cmd a pointer to character.
*
*this function has problem when terminal buffers more characters than 1024
*the while loop does not break. Instead the function getcmd() gets called
*multiple times
* @return int the number of characters read (excluding new line and EOF)
* Maximum characters read is capped at MAX_LINE_LENGTH.
*/
static int getcmd(char *cmd)
{
	int ch = EOF;
	int l = 0;
	printf("%s", PROMPT_STR);
	while(1) {
		ch = fgetc(stdin);
		if ((ch == EOF) || (l == MAX_LINE_LENGTH -1) || (ch == '\n'))
			break;
		*cmd++ = ch;
		l++;
	}
	*cmd = '\0';
	return l;
}

/**
* @brief parse the command and run idme fastboot command
* @param cmd a pointer to character
*
* parses the read buffer. It checks for syntax of the command and
* calls the idme fastboot command implementation function.
* @return int value.
*/
static int idme_process_cmd(const char *cmd)
{
	char tag[32];
	int i = 0;
	/* remove leading white spaces from command */
	while (*cmd == ' ') cmd++;
	/* read the starting tag from the command */
	while ((*cmd != ' ') && (*cmd != '\0') && (i < 31)) {
		tag[i] = *cmd;
		i++;
		cmd++;
	}

	tag[i] = '\0';

	if (strcmp(tag, CMD_TAG)) {
		dprintf(CRITICAL,"Invalid Command Syntax\n");
		return -1;
	}
	/* call idme fastboot command function and return the returned value */
	return fastboot_idme(cmd);
}

/*
Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "idme.h"

#define IDME_BLOB_FILE_PATH   "idmeblob"

int idme_platform_read(unsigned char *pbuf)
{
	int fd = -1, rc = 0;

	printf("idme_platform_read: opening the file (%s)\n",IDME_BLOB_FILE_PATH);
	if ((fd = open(IDME_BLOB_FILE_PATH, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open %s!\n",
			IDME_BLOB_FILE_PATH);
		return -1;
	}
	printf("idme_platform_read: file open successful\n");
	printf("idme_platform_read: reading %d bytes\n",CONFIG_IDME_SIZE);
	rc = read(fd, pbuf, CONFIG_IDME_SIZE);
	printf("idme_platform_read: read %d bytes\n",rc);
	/* flush to disk */
	fsync(fd);
	close(fd);

	return (rc != CONFIG_IDME_SIZE);
}
int idme_platform_write(const unsigned char *pbuf)
{
	int fd = -1, rc = 0;

	if ((fd = open(IDME_BLOB_FILE_PATH, O_WRONLY)) < 0) {
		fprintf(stderr, "Could not open %s!\n",
			IDME_BLOB_FILE_PATH);
		return -1;
	}

	rc = write(fd, pbuf, CONFIG_IDME_SIZE);

	/* flush to disk */
	fsync(fd);
	close(fd);

	return (rc != CONFIG_IDME_SIZE);
}

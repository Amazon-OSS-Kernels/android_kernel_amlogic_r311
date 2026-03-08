/* Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved. */

//This program is to be run from the AFL directory.
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "ufbl_debug.h"
#include "amzn_secure_boot.h"
#include "sig_tests_errors.h"

int amzn_image_verify(const void *image,
		  unsigned char *signature,
		  unsigned int image_size, meta_data_handler handler);
int amzn_target_device_type(void) {
	return AMZN_ENGINEERING_DEVICE;
}

//This is to store the device type that is passed as a user argument
char *device_type;
const char *amzn_target_device_name() {
	return device_type;
}

//First input argument is the offset to be fuzzed
//Second input argument is how long the section to be fuzzed is
//Third argument is the size of signature (device page size)
//Fourth input argument is the device type
//Fifth input argument is the path to the input file containing the fuzzed
//portion of the device signature
int main(int argc, char **argv) {
	int error = 0;
	if (argc < 6) {
		error = NOT_ENOUGH_INPUTS;
		goto error_handle;
	}
	unsigned char *img;
	unsigned char *sig;
	int i = 0;
	int shift = 0;
	int read_length = 0;
	int img_size = 0;
	int sig_size = 0;
	char path[40];
	if ((sscanf (argv[1], "%i", &shift) != 1) ||
			(sscanf (argv[2], "%i", &read_length) != 1) ||
			(sscanf (argv[3], "%i", &sig_size) != 1)) {
		exit(FIRST_THREE_INPUTS_NOT_INTEGERS);
	}


	if (shift < 0 || shift > sig_size ||
			read_length < 0 || read_length > sig_size ||
			(shift + read_length) > sig_size ||
			sig_size < 0) {
		error = INPUT_PARAMETERS_OUT_OF_BOUNDS;
		goto error_handle;
	}
	device_type = argv[4];
	//Construct path to appropriate boot.img
	strncpy(path,"../include/",11);
	strcat(path,device_type);
	strcat(path,"/boot.img");
	FILE *amzn_boot_img = fopen( path, "r" );
	FILE *amzn_sig_img = fopen( argv[5], "r" );
	if (amzn_boot_img == NULL || amzn_sig_img == NULL) {
		if (amzn_boot_img != NULL) fclose(amzn_boot_img);
		if (amzn_sig_img != NULL) fclose(amzn_sig_img);
		error = CANNOT_OPEN_FILE;
		goto error_handle;
	}
	//Determine size of file, limit 2GB
	//A bootloader image will never be that big
	fseek(amzn_boot_img, 0L, SEEK_END);
	img_size = ftell(amzn_boot_img);
	rewind(amzn_boot_img);
	img = malloc(img_size);
	if (img == NULL) {
		error = UNABLE_TO_ALLOCATE_MEMORY;
		goto error_handle;
	}
	sig = img;
	sig += img_size-sig_size;

	fread(img, 1, img_size, amzn_boot_img);
	fread(sig+shift, 1, read_length, amzn_sig_img);
	fclose(amzn_boot_img);
	fclose(amzn_sig_img);


	if (amzn_image_verify(img, sig, img_size-sig_size, 0)) {
		printf("Image validated\n");
		free(img);
		return 0;
	}
	else {
		printf("Image invalid\n");
		free(img);
		return 1;
	}
error_handle:
	switch(error) {
	case NOT_ENOUGH_INPUTS:
		fprintf(stderr,"error - not enough input arguments.\n");
		fprintf(stderr,"Please give arguments: offset, no. of bytes,"
			" device type, path to file containing signature\n");
		break;
	case FIRST_THREE_INPUTS_NOT_INTEGERS:
		fprintf(stderr, "error - first three inputs not integers.\n");
		break;
	case INPUT_PARAMETERS_OUT_OF_BOUNDS:
		fprintf(stderr, "error - input parameters out of bounds.\n");
		break;
	case CANNOT_OPEN_FILE:
		fprintf(stderr,"error - file was not opened.\n");
		break;
	case UNABLE_TO_ALLOCATE_MEMORY:
		fprintf(stderr,"error - memory not successfully allocated\n");
		break;
	default:
		fprintf(stderr,"An error has occurred\n");

	}
}

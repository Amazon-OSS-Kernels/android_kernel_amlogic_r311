/*
Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*/

#include <stdio.h>
#include "ufbl_debug.h"
#include "idme.h"
#include "itt.h"

int main(void)
{
	dprintf(CRITICAL, "IDME Functional Tool v1.0\n");
	idme_initialize();
	itt_idme_process();
	dprintf(CRITICAL, "IDME Functional tool exiting.\n");
	return 0;
}


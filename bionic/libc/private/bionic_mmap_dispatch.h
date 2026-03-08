/*
 * Copyright (c) 2023 Amazon.com, Inc. or its affiliates.  All rights reserved.
 *
 * PROPRIETARY/CONFIDENTIAL.  USE IS SUBJECT TO LICENSE TERMS.
 */

#ifndef _PRIVATE_BIONIC_MMAP_DISPATCH_H
#define _PRIVATE_BIONIC_MMAP_DISPATCH_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <private/bionic_config.h>

// Entry in malloc dispatch table.
typedef void* (*MmapMmap)(void*, size_t, int, int, int, off_t);
typedef int (*MmapMunmap)(void*, size_t);

struct MmapDispatch {
  MmapMunmap munmap;
  MmapMmap mmap;
} __attribute__((aligned(32)));

#endif

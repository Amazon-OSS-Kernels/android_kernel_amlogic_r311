/*
 * Copyright (C) 2015 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions of this file are copyright (c) 2023 Amazon.com, Inc. or its affiliates.  All rights reserved.
 *
 * PORTIONS OF THIS FILE ARE AMAZON PROPRIETARY/CONFIDENTIAL.  USE IS SUBJECT TO LICENSE TERMS.
 *
 * Amazon modifications are indicated by [fosmod_* comments].
 */

#ifndef MMAP_DEBUG_H
#define MMAP_DEBUG_H

#include <fireos/FosMods.h>     // fosmod_fosmods oneline

/* fosmod_memleak_debug begin */
#if defined(FOSMOD_MEMLEAK_DEBUG)
#include <stdint.h>

#include <private/bionic_mmap_dispatch.h>

extern const MmapDispatch* g_mmap_dispatch;

extern int  gMemleakDumpFileIndex;
extern char gMemleakDumpHeapFile[];
extern void dumpMemleakProcessHeap(const char *file, int idx);
#endif
/* fosmod_memleak_debug end */

#endif // MMAP_DEBUG_H

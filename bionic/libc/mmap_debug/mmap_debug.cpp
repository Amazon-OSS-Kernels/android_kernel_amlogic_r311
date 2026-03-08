/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <unistd.h>

#include <fireos/FosMods.h>     // fosmod_fosmods oneline

#include <private/bionic_mmap_dispatch.h>

#include "backtrace.h"
#include "DebugData.h"
#include "debug_disable.h"
#include "debug_log.h"
#include "mmap_debug.h"

// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Use C style prototypes for all exported functions. This makes it easy
// to do dlsym lookups during libc initialization when mmap debug
// is enabled.
// ------------------------------------------------------------------------
__BEGIN_DECLS

bool debug_initialize_mmap(const MmapDispatch* mmap_dispatch, int* mmap_zygote_child,
    const char* options);
void debug_finalize_mmap();
void* debug_mmap(void *addr, size_t size,
    int prot, int flags, int fd, off_t offset);
int debug_munmap(void* pointer, size_t len);

__END_DECLS
// ------------------------------------------------------------------------


/* fosmod_memleak_debug begin */
#if defined(FOSMOD_MEMLEAK_DEBUG)
// ------------------------------------------------------------------------
// Global Data
// ------------------------------------------------------------------------
extern DebugData* g_debug;

int* g_mmap_zygote_child;

const MmapDispatch* g_mmap_dispatch;

#include <sys/system_properties.h>

static const char* DEBUG_MMAPLEAK_DUMPHEAP_FILE = "libc.debug.mmapleak.file";

static void InitAtfork() {
  static pthread_once_t atfork_init = PTHREAD_ONCE_INIT;
  pthread_once(&atfork_init, [](){
    pthread_atfork(
        [](){
          if (g_debug != nullptr) {
            g_debug->PrepareFork();
          }
        },
        [](){
          if (g_debug != nullptr) {
            g_debug->PostForkParent();
          }
        },
        [](){
          if (g_debug != nullptr) {
            g_debug->PostForkChild();
          }
        }
    );
  });
}

static void LogUnmapError(size_t length, const void* pointer, const char* name) {
  error_log(LOG_DIVIDER);
  error_log("+++ MMAP %p PARTIAL MUNMAP length %zu (%s)", pointer, length, name);
  error_log(LOG_DIVIDER);
}

static void* InitHeader(Header* header, void* orig_pointer, size_t size) {
  header->tag = DEBUG_TAG;
  header->orig_pointer = orig_pointer;
  header->size = size;
  if (*g_mmap_zygote_child) {
    header->set_zygote();
  }

  bool backtrace_found = false;
  if (g_debug->config().options & BACKTRACE) {
    BacktraceHeader* back_header = g_debug->GetAllocBacktrace(header);
    if (g_debug->backtrace->enabled()) {
      back_header->num_frames = backtrace_get(
          &back_header->frames[0], g_debug->config().backtrace_frames);
      backtrace_found = back_header->num_frames > 0;
    } else {
      back_header->num_frames = 0;
    }
  }

  if (g_debug->config().options & TRACK_ALLOCS) {
    g_debug->track->Add(header, backtrace_found);
  }

  return g_debug->GetPointer(header);
}

bool debug_initialize_mmap(const MmapDispatch* mmap_dispatch, int* mmap_zygote_child,
    const char* options) {
  if (mmap_zygote_child == nullptr || options == nullptr) {
    return false;
  }

  InitAtfork();

  g_mmap_zygote_child = mmap_zygote_child;

  g_mmap_dispatch = mmap_dispatch;

  char value[PROP_VALUE_MAX];
  if (__system_property_get(DEBUG_MMAPLEAK_DUMPHEAP_FILE, value) != 0) {
    strncpy(gMemleakDumpHeapFile, value, PROP_VALUE_MAX);
    gMemleakDumpFileIndex = 1;
  } else {
    error_log("Unable to find prop %s", DEBUG_MMAPLEAK_DUMPHEAP_FILE);
  }

  if (!DebugDisableInitialize()) {
    return false;
  }

  DebugData* debug = new DebugData();
  if (!debug->Initialize(options)) {
    delete debug;
    DebugDisableFinalize();
    return false;
  }
  g_debug = debug;

  // Always enable the backtrace code since we will use it in a number
  // of different error cases.
  backtrace_startup();

  return true;
}

void debug_finalize_mmap() {
  if (g_debug == nullptr) {
    return;
  }

  DebugDisableSet(true);

  backtrace_shutdown();

  delete g_debug;
  g_debug = nullptr;

  DebugDisableFinalize();
}

static void *internal_mmap(void *addr, size_t size,
    int prot, int flags, int fd, off_t offset) {

  // real size + meta
  size_t total_size = size + g_debug->extra_bytes();

  if (total_size < size) {
    // Overflow.
    errno = ENOMEM;
    return nullptr;
  }

  void* pointer;
  if (g_debug->need_header()) {
    if (size > Header::max_size()) {
      errno = ENOMEM;
      return nullptr;
    }

    Header* header = reinterpret_cast<Header*>(
        g_mmap_dispatch->mmap(addr, total_size, prot, flags, fd, offset));
    if (header == nullptr) {
      return nullptr;
    }

    header->usable_size = total_size;

    pointer = InitHeader(header, header, size);
  } else {
    pointer = g_mmap_dispatch->mmap(addr, total_size, prot, flags, fd, offset);
  }

  return pointer;
}

void* debug_mmap(void *addr, size_t size,
    int prot, int flags, int fd, off_t offset) {
  if (DebugCallsDisabled()) {
    return g_mmap_dispatch->mmap(addr, size, prot, flags, fd, offset);
  }
  ScopedDisableDebugCalls disable;

  void* pointer = internal_mmap(addr, size,
                                prot, flags, fd, offset);
  return pointer;
}

static int internal_munmap(void* pointer, size_t len) {
  int ret = 0;
  void* munmap_pointer = pointer;

  if (g_debug->need_header()) {
    Header* header = nullptr;
    header = g_debug->track->GetHeader(pointer, len);
    if (header == nullptr) {
      LogUnmapError(len, pointer, "unmap");
      return -1;
    }

    if (header->size != len) {
      ret = g_mmap_dispatch->munmap(munmap_pointer, len);
      if (ret < 0) {
        LogUnmapError(len, pointer, "unmap");
      } else {
        header->size -= len;
      }
      return ret;
    }

    len = header->usable_size;
    munmap_pointer = header->orig_pointer;

    if (g_debug->config().options & TRACK_ALLOCS) {
      bool backtrace_found = false;
      if (g_debug->config().options & BACKTRACE) {
        BacktraceHeader* back_header = g_debug->GetAllocBacktrace(header);
        backtrace_found = back_header->num_frames > 0;
      }
      g_debug->track->Remove(header, backtrace_found);
    }

    /* Unmap header */
    ret = g_mmap_dispatch->munmap(munmap_pointer, len);
    return ret;
  }

  return g_mmap_dispatch->munmap(munmap_pointer, len);
}

int debug_munmap(void* pointer, size_t len) {
  if (DebugCallsDisabled() || pointer == nullptr) {
    return g_mmap_dispatch->munmap(pointer, len);
  }
  ScopedDisableDebugCalls disable;

  return internal_munmap(pointer, len);
}
#else
bool debug_initialize_mmap(
    __attribute__((unused)) const MmapDispatch* mmap_dispatch,
    __attribute__((unused)) int* mmap_zygote_child,
    __attribute__((unused)) const char* options) {
        return false;
}

void debug_finalize_mmap() {
}

void* debug_mmap(
    __attribute__((unused)) void *addr,
    __attribute__((unused)) size_t size,
    __attribute__((unused)) int prot,
    __attribute__((unused)) int flags,
    __attribute__((unused)) int fd,
    __attribute__((unused)) off_t offset) {
        return nullptr;
}

int debug_munmap(
    __attribute__((unused)) void* pointer,
    __attribute__((unused)) size_t len) {
    return -1;
}
#endif
/* fosmod_memleak_debug end */

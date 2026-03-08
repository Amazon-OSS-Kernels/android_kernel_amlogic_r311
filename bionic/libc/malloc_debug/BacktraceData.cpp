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
 * BacktraceData.cpp
 *
 * Copyright (c) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * Use is subject to license terms.
 * Changes introduced by Amazon.com, Inc. or its affiliates are indicated by
 * fosmod_* comments.
 */

#include <thread>
#include <mutex>
#include <condition_variable>

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <private/bionic_macros.h>

#include <fireos/FosMods.h>     // fosmod_fosmods oneline

#include "BacktraceData.h"
#include "Config.h"
#include "DebugData.h"
#include "debug_log.h"
#include "malloc_debug.h"

/* fosmod_memleak_debug begin */
#if defined(FOSMOD_MEMLEAK_DEBUG)
/* heapdump lock */
std::mutex heapdump_lock;

/* heapdump cond cv */
std::condition_variable heapdump_cond;

static void heapdump_func() {
  std::unique_lock<std::mutex> lck(heapdump_lock);
  while (true) {
    heapdump_cond.wait(lck);
    dumpMemleakProcessHeap(gMemleakDumpHeapFile, gMemleakDumpFileIndex);
    gMemleakDumpFileIndex++;
  }
}
#endif
/* fosmod_memleak_debug end */

static void EnableToggle(int, siginfo_t*, void*) {
  if (g_debug->backtrace->enabled()) {
    g_debug->backtrace->set_enabled(false);
    /* fosmod_memleak_debug begin */
#if defined(FOSMOD_MEMLEAK_DEBUG)
    if (gMemleakDumpFileIndex) {
      std::unique_lock<std::mutex> lck(heapdump_lock);
      heapdump_cond.notify_one();
      info_log("%s: DumpHeap: in file %s%d",
                  getprogname(), gMemleakDumpHeapFile, gMemleakDumpFileIndex);
      info_log("%s: Run: 'kill -45 %d' to enable backtracing.",
                  getprogname(), getpid());
    }
#endif
    /* fosmod_memleak_debug end */
  } else {
    g_debug->backtrace->set_enabled(true);
  }
}

BacktraceData::BacktraceData(DebugData* debug_data, const Config& config, size_t* offset)
    : OptionData(debug_data) {
  size_t hdr_len = sizeof(BacktraceHeader) + sizeof(uintptr_t) * config.backtrace_frames;
  alloc_offset_ = *offset;
  *offset += BIONIC_ALIGN(hdr_len, MINIMUM_ALIGNMENT_BYTES);
}

bool BacktraceData::Initialize(const Config& config) {
  /* fosmod_memleak_debug begin */
#if defined(FOSMOD_MEMLEAK_DEBUG)
  if (gMemleakDumpFileIndex) {
    /* heapdump thread */
    static std::thread heapdumpThread(heapdump_func);
  }
#endif
  /* fosmod_memleak_debug end */
  enabled_ = config.backtrace_enabled;
  if (config.backtrace_enable_on_signal) {
    struct sigaction enable_act;
    memset(&enable_act, 0, sizeof(enable_act));

    enable_act.sa_sigaction = EnableToggle;
    enable_act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&enable_act.sa_mask);
    if (sigaction(config.backtrace_signal, &enable_act, nullptr) != 0) {
      error_log("Unable to set up backtrace signal enable function: %s", strerror(errno));
      return false;
    }
    info_log("%s: Run: 'kill -%d %d' to enable backtracing.", getprogname(),
             config.backtrace_signal, getpid());
  }
  return true;
}

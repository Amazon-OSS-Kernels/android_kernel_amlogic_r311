/*
 * Copyright (c) 2023 Amazon.com, Inc. or its affiliates.  All rights reserved.
 *
 * PROPRIETARY/CONFIDENTIAL.  USE IS SUBJECT TO LICENSE TERMS.
 */

#include <pthread.h>

#include <private/bionic_config.h>
#include <private/bionic_globals.h>
#include <private/bionic_mmap_dispatch.h>

extern "C" int  __munmap(void*, size_t); // fosmod_memleak_debug oneline

#define Mmapped(function) __ ## function ## 1

// MmapDispatch table entries below maps
// mmap call to Mmapped(mmap) -> __mmap1
// munmap call to Mmapped(mmap) -> __munmap1

static constexpr MmapDispatch __libc_mmap_default_dispatch
  __attribute__((unused)) = {
    Mmapped(munmap),
    Mmapped(mmap),
  };

// In a VM process, this is set to 1 after fork()ing out of zygote.
int gMmapLeakZygoteChild = 0;

// =============================================================================
// mmap functions
// =============================================================================

void* __mmap1(void* addr, size_t size, int prot, int flags, int fd, off_t offset) {
  return mmap64(addr, size, prot, flags, fd, static_cast<off64_t>(offset));
}

int __munmap1(void* addr, size_t size) {
  return __munmap(addr, size);
}

extern "C" int __attribute__((weak)) munmap(void* mem, size_t bytes) {
  int ret;
  auto _munmap = __libc_globals->mmap_dispatch.munmap;
  if (__predict_false(_munmap != nullptr)) {
    ret = _munmap(mem, bytes);
  } else {
    ret = Mmapped(munmap)(mem, bytes);
  }

  return ret;
}

extern "C" void* __attribute__((weak)) mmap(void *mem, size_t bytes,
                  int prot, int flags,
                  int fd, off_t offset) {
  auto _mmap = __libc_globals->mmap_dispatch.mmap;
  if (__predict_false(_mmap != nullptr)) {
    return _mmap(mem, bytes, prot, flags, fd, offset);
  }
  return  Mmapped(mmap)(mem, bytes, prot, flags, fd, offset);
}

// We implement mmap debugging only in libc.so, so the code below
// must be excluded if we compile this file for static libc.a
#if !defined(LIBC_STATIC)

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include <private/libc_logging.h>
#include <sys/system_properties.h>

extern "C" int __cxa_atexit(void (*func)(void *), void *arg, void *dso);

static const char* DEBUG_SHARED_LIB = "libc_mmap_debug.so";
static const char* DEBUG_MMAP_PROPERTY_OPTIONS = "libc.debug.mmap.options";
static const char* DEBUG_MMAP_PROPERTY_PROGRAM = "libc.debug.mmap.program";
static const char* DEBUG_MMAP_ENV_OPTIONS = "LIBC_DEBUG_MMAP_OPTIONS";

static void* libc_mmap_impl_handle = nullptr;

static void (*g_debug_finalize_func)();

// =============================================================================
// Log functions
// =============================================================================
#define error_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_ERROR, "libc", (format), ##__VA_ARGS__ )
#define info_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_INFO, "libc", (format), ##__VA_ARGS__ )
// =============================================================================

// Retrieve native heap information.
//
// "*info" is set to a buffer we allocate
// "*overall_size" is set to the size of the "info" buffer
// "*info_size" is set to the size of a single entry
// "*total_memory" is set to the sum of all mmap we're tracking; does
//   not include heap overhead
// "*backtrace_size" is set to the maximum number of entries in the back trace

// =============================================================================

template<typename FunctionType>
static bool InitMmapFunction(void* mmap_impl_handler, FunctionType* func, const char* prefix, const char* suffix) {
  char symbol[128]; // contains prefix (debug) and suffix (mmap/munmap), see below
  snprintf(symbol, sizeof(symbol), "%s_%s", prefix, suffix);
  *func = reinterpret_cast<FunctionType>(dlsym(mmap_impl_handler, symbol));
  if (*func == nullptr) {
    error_log("%s: dlsym(\"%s\") failed", getprogname(), symbol);
    return false;
  }
  return true;
}

static bool InitMmap(void* mmap_impl_handler, MmapDispatch* table, const char* prefix) {
  if (!InitMmapFunction<MmapMunmap>(mmap_impl_handler, &table->munmap,
                                      prefix, "munmap")) {
    return false;
  }
  if (!InitMmapFunction<MmapMmap>(mmap_impl_handler, &table->mmap,
                                        prefix, "mmap")) {
    return false;
  }

  return true;
}

static void mmap_fini_impl(void*) {
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);

  g_debug_finalize_func();
}

// Initializes memory mmap framework once per process.
static void mmap_init_impl(libc_globals* globals) {
  char value[PROP_VALUE_MAX];

  // If DEBUG_MMAP_ENV_OPTIONS is set then it overrides the system properties.
  const char* options = getenv(DEBUG_MMAP_ENV_OPTIONS);
  if (options == nullptr || options[0] == '\0') {
    if (__system_property_get(DEBUG_MMAP_PROPERTY_OPTIONS, value) == 0 || value[0] == '\0') {
      return;
    }
    const char* mmap_str = " mmap_track";
    strncat(value, mmap_str, PROP_VALUE_MAX - strlen(value) - 1);
    options = value;

    // Check to see if only a specific program should have debug mmap enabled.
    char program[PROP_VALUE_MAX];
    if (__system_property_get(DEBUG_MMAP_PROPERTY_PROGRAM, program) != 0 &&
        strstr(getprogname(), program) == nullptr) {
      return;
    }
  }

  // Load the debug mmap shared library.
  void* mmap_impl_handle = dlopen(DEBUG_SHARED_LIB, RTLD_NOW | RTLD_LOCAL);
  if (mmap_impl_handle == nullptr) {
    error_log("%s: Unable to open debug mmap shared library %s: %s",
              getprogname(), DEBUG_SHARED_LIB, dlerror());
    return;
  }

  // Initialize mmap debugging in the loaded module.
  auto init_func = reinterpret_cast<bool (*)(const MmapDispatch*, int*, const char*)>(
      dlsym(mmap_impl_handle, "debug_initialize_mmap"));
  if (init_func == nullptr) {
    error_log("%s: debug_initialize routine not found in %s", getprogname(), DEBUG_SHARED_LIB);
    dlclose(mmap_impl_handle);
    return;
  }

  // Get the syms for the external functions.
  void* finalize_sym = dlsym(mmap_impl_handle, "debug_finalize_mmap");
  if (finalize_sym == nullptr) {
    error_log("%s: debug_finalize routine not found in %s", getprogname(), DEBUG_SHARED_LIB);
    dlclose(mmap_impl_handle);
    return;
  }

  if (!init_func(&__libc_mmap_default_dispatch, &gMmapLeakZygoteChild, options)) {
    dlclose(mmap_impl_handle);
    return;
  }

  MmapDispatch mmap_dispatch_table;
  if (!InitMmap(mmap_impl_handle, &mmap_dispatch_table, "debug")) {
    auto finalize_func = reinterpret_cast<void (*)()>(finalize_sym);
    finalize_func();
    dlclose(mmap_impl_handle);
    return;
  }

  g_debug_finalize_func = reinterpret_cast<void (*)()>(finalize_sym);

  globals->mmap_dispatch = mmap_dispatch_table;
  libc_mmap_impl_handle = mmap_impl_handle;

  info_log("%s: mmap debug enabled", getprogname());

  int ret_value = __cxa_atexit(mmap_fini_impl, nullptr, nullptr);
  if (ret_value != 0) {
    error_log("failed to set atexit cleanup function: %d", ret_value);
  }
}

__LIBC_HIDDEN__ void __libc_init_mmap(libc_globals* globals) {
  mmap_init_impl(globals);
}
#endif  // !LIBC_STATIC

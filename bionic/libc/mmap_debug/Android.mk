LOCAL_PATH := $(call my-dir)

libc_mmap_debug_src_files := \
    ../malloc_debug/BacktraceData.cpp \
    ../malloc_debug/Config.cpp \
    ../malloc_debug/DebugData.cpp \
    ../malloc_debug/debug_disable.cpp \
    ../malloc_debug/FreeTrackData.cpp \
    ../malloc_debug/GuardData.cpp \
    ../malloc_debug/malloc_debug.cpp \
    ../malloc_debug/RecordData.cpp \
    ../malloc_debug/TrackData.cpp \
    mmap_debug.cpp \

# ==============================================================
# libc_mmap_debug_backtrace.a
# ==============================================================
# Used by libmemunreachable
include $(CLEAR_VARS)

LOCAL_MODULE := libc_mmap_debug_backtrace

LOCAL_SRC_FILES := \
    ../malloc_debug/backtrace.cpp \
    ../malloc_debug/MapData.cpp \

LOCAL_CXX_STL := libc++_static

LOCAL_STATIC_LIBRARIES += \
    libc_logging \

LOCAL_C_INCLUDES += bionic/libc
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)

LOCAL_SANITIZE := never
LOCAL_NATIVE_COVERAGE := false

# -Wno-error=format-zero-length needed for gcc to compile.
LOCAL_CFLAGS := \
    -Wall \
    -Werror \
    -Wno-error=format-zero-length \

include $(BUILD_STATIC_LIBRARY)

# ==============================================================
# libc_mmap_debug.so
# ==============================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libc_mmap_debug

LOCAL_SRC_FILES := \
    $(libc_mmap_debug_src_files) \

LOCAL_CXX_STL := libc++_static

# Only need this for arm since libc++ uses its own unwind code that
# doesn't mix with the other default unwind code.
LOCAL_STATIC_LIBRARIES_arm := libunwind_llvm

LOCAL_STATIC_LIBRARIES += \
    libbase \
    libc_mmap_debug_backtrace \
    libc_logging \

LOCAL_LDFLAGS_32 := -Wl,--version-script,$(LOCAL_PATH)/exported32.map
LOCAL_LDFLAGS_64 := -Wl,--version-script,$(LOCAL_PATH)/exported64.map
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_C_INCLUDES += bionic/libc bionic/libc/malloc_debug

LOCAL_SANITIZE := never
LOCAL_NATIVE_COVERAGE := false

# -Wno-error=format-zero-length needed for gcc to compile.
LOCAL_CFLAGS := \
    -Wall \
    -Werror \
    -fno-stack-protector \
    -Wno-error=format-zero-length \

include $(BUILD_SHARED_LIBRARY)

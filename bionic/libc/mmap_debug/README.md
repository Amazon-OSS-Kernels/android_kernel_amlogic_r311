Mmap Debug
==========

Mmap debug is a method of debugging native mmap problems. It can help
detect mmap calls and let user detect any that was not unmapped.

This documentation describes how to enable this feature on Puffin Stack.

In order to enable mmap debug, you must be able to set special system
properties using the setprop command from the shell. This requires the
ability to run as root on the device.

When mmap debug is enabled, it works by adding a shim layer that replaces
the normal mmap calls. The replaced calls are:

* `mmap`
* `munmap`

Any errors detected by the library are reported in the log.

Controlling Mmap Debug Behavior
---------------------------------
Mmap debug is controlled by individual options. Each option can be enabled
individually, or in a group of other options. Every single option can be
combined with every other option.

Option Descriptions
-------------------
### backtrace[=MAX\_FRAMES]
Enable capturing the backtrace of each mmap site.
This option will slow down mmap by an order of magnitude. If the
system runs too slowly with this option enabled, decreasing the maximum number
of frames captured will speed the mmap up.

If MAX\_FRAMES is present, it indicates the maximum number of frames to
capture in a backtrace. The default is 16 frames, the maximumum value
this can be set to is 256.

This option adds a special header to all mmap that contains the
backtrace and information about the original mmap.

### backtrace\_enable\_on\_signal[=MAX\_FRAMES]
Enable capturing the backtrace of each mmap site. If the
backtrace capture is toggled when the process receives the signal
SIGRTMAX - 19 (which is 45 on most Android devices). When this
option is used alone, backtrace capture starts out disabled until the signal
is received. If both this option and the backtrace option are set, then
backtrace capture is enabled until the signal is received.

If MAX\_FRAMES is present, it indicates the maximum number of frames to
capture in a backtrace. The default is 16 frames, the maximumum value
this can be set to is 256.

Wiki Link:

www.amazon.com

Steps:

To enable mmap leak tracing:

setprop libc.debug.mmap.program <name of executable to trace>
setprop libc.debug.mmap.options backtrace_enable_on_signal=4
setprop libc.debug.mmapleak.file mmapleak
setenforce 0
chmod 777 /data
Kill -9 <PID of executable to trace>
Re-execute <executable to trace>

kill -45 <PID of executable to trace>

Starts collecting backtraces of mmap() call with no
corresponding unmap() call made by the executable

After sometime issue:

kill -45 <PID of executable to trace>

To stop the collection and send the outstanding mmap()
call with backtrace to file /data/mmapleak:0

You can repeat the above two kill steps to collect
more backtraces of outstanding mmaps calls made by
the executable.

Next in you host system issue:

adb pull /data/mmapleak:0

Run,

python ~/debug/native_dump_viewer.py --symbols <path>  ./mmapleak\:0

native_dump_viewer.py script is in google search OR
www.amazon.com


For example,

python ~/debug/mmap_dump_viewer.py --symbols /bld1/fos6/out/target/product/abh123_puffin/symbols ./mmapleak\:0

Resolving symbols using directory /bld1/fos6/out/target/product/abh123_puffin/symbols...

    BYTES %TOTAL %PARENT    COUNT    ADDR LIBRARY FUNCTION LOCATION
        0   0.00%   0.00%        0 APP

    16384 100.00% 100.00%        2 ZYGOTE
    16384 100.00% 100.00%        2   b6d22214 /system/lib/libc.so __libc_init /proc/self/cwd/bionic/libc/bionic/libc_init_dynamic.cpp:110
    12288  75.00%  75.00%        1     b6f533e0 /system/bin/mmapleak main /proc/self/cwd/bionic/libc/mmap_debug/tests/mmapleak.cpp:104
    12288  75.00% 100.00%        1       b6f531b0 /system/bin/mmapleak mmap_pages(int) /proc/self/cwd/bionic/libc/mmap_debug/tests/mmapleak.cpp:76 (discriminator 1)
     4096  25.00%  25.00%        1     b6f533ee /system/bin/mmapleak main /proc/self/cwd/bionic/libc/mmap_debug/tests/mmapleak.cpp:106
     4096  25.00% 100.00%        1       b6f531b0 /system/bin/mmapleak mmap_pages(int) /proc/self/cwd/bionic/libc/mmap_debug/tests/mmapleak.cpp:76 (discriminator 1)


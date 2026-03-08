signature_test_tool, is a tool that can be used to perform regression tests to
make sure changes to validation code still work properly.
This tool is to be run on a host device. This tool additionally, in its
AFL directory has a tool to perform fuzzing operations on
the boot image's signature.
This tool can check boot images independant of the lunch option chosen
when setting up your environment.
This tool currently only works on x86 hosts using Linux
The libcrypto.a and libssl.a libraries do not have a compilation process
right now. They are precompiled openssl libraries downloaded from online

**regression_tests**
Going in this directory and running make will compile functional tests
	with the validation code to check for various things. As of right now,
	tests include testing for NULL, various bounds testing, and providing
	the validation code with bad images/signatures.
To run:
	cd regression_tests
	'make openssl' OR 'make libtomcrypt'
	./run.sh <Device Type> (ex ./run.sh ABC)
To clean:
	cd regression_tests
	make clean

**AFL**
AFL (American Fuzzy Lop) is fuzzing software that can be
executed with binaries/executables.
Entering this directory running the script run.sh will invoke the makefile
and begin running multiple instances of AFL to fuzz over every
header section in the boot.img signature
Running run.sh will generate many files under the /testcases/<Device_Type> and
/output/<Device_Type> directories. These are named the byte offset
of the header being fuzzed, relative to the beginning of the signature
which is the last page size of bytes of boot.img
To access stats gathered from the fuzzer, go to AFL/output.
Each device directory should contain a crash_report.txt file
which will have a list of all offsets that have a crash
Each directory in this one will contain a fuzzer_stats file
The crash folder has files containing the input that was passed
to cause the crash (if one was caused). The crash directory
will only retain the first set of inputs that caused each
unique crash.
To run/make:
	cd AFL
	'make openssl' OR 'make libtomcrypt'
	./run.sh <Device Type> <Run_time> (ex ./run.sh ABC 5)
To clean:
	cd AFL
	make clean
Customize:
	Adjusting the <Run_time> input can change how long
	(in seconds) you want to fuzz each header

**Adding new devices**
If the device type you want to test is not in the /include directory,
download the latest userdebug build from KBITS, extract it,
copy the 'boot.img' located in it, and make a new directory
in /include named after the device and insert
the boot.img file in to it.

**Updating boot.img files**
Follow the instructions for 'Adding new devices', but instead of
creating a new directory in /include, simply go in there,
find the device type of the matching name, and replace
the old boot.img file with the new one.

**Compiling libtomcrypt**
	cd lib
	./make_libtomcrypt.sh
-g can be added in the list of CFLAGS in the script for debugging

**Compiling libtommath**
	cd lib
	./make_libtommath.sh
-g can be added in the list of CFLAGS in the script for debugging

#
# Copyright 2019 Amazon.com, Inc. or its Affiliates. All rights reserved.
#

cd ../../../../../../external/libtommath
rm -f libtommath.a
CFLAGS="" make
cp libtommath.a ../../bootable/bootloader/ufbl-features/tests/signature_test_tool/lib

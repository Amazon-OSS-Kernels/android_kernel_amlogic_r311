#
# Copyright 2019 Amazon.com, Inc. or its Affiliates. All rights reserved.
#
cd ../../../../../../external/libtomcrypt
rm -f libtomcrypt.a
CFLAGS="-DLTM_DESC -DUSE_LTM -DLTC_SOURCE -DCONFIG_UFBL_FUZZER -DLTC_NO_FILE -DUSE_LTM -DLTM_DESC -DLTC_CLEAN_STACK -DCONFIG_UFBL -g" make
cp libtomcrypt.a ../../bootable/bootloader/ufbl-features/tests/signature_test_tool/lib

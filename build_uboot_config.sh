################################################################################
#
#  build_kernel_config.sh
#
#  Copyright (c) 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

UBOOT_SUBPATH="bootable/bootloader/uboot-amlogic_r311"
UBOOT_DEFCONFIG_NAME="blanche_config"
export TARGET_PRODUCT_NAME_BLANCHE=y

# Expected image files are seperated with ":"
UBOOT_IMAGES="fip/blanche/u-boot.bin"

################################################################################
# NOTE: You must fill in the following with the path to a copy of an
# gcc-linaro-aarch64-none-elf-4.8-2013.11_linux (aarch64-none-elf compiler) and
# CodeSourcery g++ lite (arm-none-eabi compiler)
################################################################################
export PATH="$PATH:<path/to/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin>"
export PATH="$PATH:<path/to/Sourcery_G++_Lite/bin>"

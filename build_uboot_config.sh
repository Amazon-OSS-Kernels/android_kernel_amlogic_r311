################################################################################
#
#  build_kernel_config.sh
#
#  Copyright (c) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

UBOOT_SUBPATH="bootable/bootloader/uboot-amlogic_r311"
UBOOT_DEFCONFIG_NAME="blanche_defconfig"
export TARGET_PRODUCT_NAME_RANCHO=y

# Expected image files are seperated with ":"
UBOOT_IMAGES="fip/blanche/u-boot.bin"

################################################################################
# NOTE: You must fill in the following with the path to a copy of an
#       gcc-linaro-aarch64-none-elf-4.8 (aarch64-none-elf compiler) and
#       CodeSourcery g++ lite (arm-none-eabi compiler)
################################################################################
export PATH="$PATH:<path/to/aarch64-none-elf/bin>"
export PATH="$PATH:<path/to/arm-none-eabi/bin>"

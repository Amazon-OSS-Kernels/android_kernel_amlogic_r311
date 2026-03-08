README
BUILDING UBoot

take the Almond for example

1.Obtain a copy of gcc-linaro-aarch64-none-elf-4.8 (aarch64-none-elf compiler)
	or a substitute cross-compiler. Recommended version is 2013.11. Add its bin
	path to environment variable PATH

	if you have downloaded the project code,please
	export PATH=code_path/prebuilts/gcc/linux-x86/aarch64/linaro-gcc-aarch64-4.8/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin:$PATH

2.Obtain a copy of riscv-none-gcc ( riscv-none-embed compiler)
	or a substitute cross-compiler. Recommended version is 7.2.0-4-20180606-1631. Add its bin
	path to environment variable PATH

	if you have downloaded the project code,please
	export PATH=code_path/vendor/amlogictv/v901d/common/tools/riscv-none-gcc/7.2.0-4-20180606-1631/bin:$PATH

3.command
cd code_path/bootable/bootloader/uboot-amlogic/t950x4

4.command
UFBL_PLAT_PROJ=aml_t950x4 ./mk almond --systemroot

5.out
the uboot.bin can be finded in bl33/build

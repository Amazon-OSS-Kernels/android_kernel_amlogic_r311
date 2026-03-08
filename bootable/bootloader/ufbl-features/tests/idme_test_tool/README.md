This tool tests the idme functionality as used on a real device.
However this tool can be built and run on host machines and the
tool expects the idme data be made available in a file called
`idmeblob` which should be present in the same folder as in the
tool runs. To generate IDME blob userspace tool in external/idme
can be used (command :`idme dump <filename`) 

To simulate a situation when idme data is not available (not
present in storage) then we should provide an empty file with
should be at least as large as `CONFIG_IDME_SIZE`.

To create an empty file following command can be used:
```
dd if=/dev/zero of=idmeblob bs=512 count=<no_of_blocks>
```

If you are not sure of the size of the idme data on the
device then use following as guide:

 * Each device should have `project/<product_name>.mk` in `project` directory.
check for `IDME_NUM_OF_EMMC_BLOCKS` in that file. The `no_of_blocks` is
`IDME_NUM_OF_EMMC_BLOCKS`

 *  If the size can not be determined (there is no `IDME_NUM_OF_EMMC_BLOCKS`)
then find the bootloader used on the device. If its LK then default number
of block used are 30 and then the size of the IDME data (hence file size)
is 15360 (512x30) bytes. Or else if it's uboot then the default no. block
used by idme is 10 and the IDME data takes 5120 (512x10).

USAGE if TOOL
=============

extract idme blob from some device using idme dd command

```
dd if=<path_of_idme_partition> of=idmeblob bs=512 count=<no_of_blob>
```
or use empty idmeblob as described above.
edit ```ufbl-features/project/tests.mk``` to edit ```IDME_NUM_OF_EMMC_BLOCKS``` and other macros.

rename platform default idme table in ```ufbl-features/features/idme/include/idme_default_table_xxxx.h```
to ```idme_default_table_tests.h``` in same folder.

run
```make```
which will generate idme_test_tool in ```out/```
copy the ```idmeblob``` (hard coded name) in ```out```
```cd out/``` and run ```./idme_test_tool```
run commands such as ```idme ?``` etc from command prompt.

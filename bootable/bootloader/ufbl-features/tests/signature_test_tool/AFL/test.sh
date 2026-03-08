#!/bin/sh
# Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#Test just one altered signature at a time
#Run with ./test.sh <Device_Type> <Offset> <Path_to_file>

DEVICE=$1
OFFSET=$2
FILE=$3
if [ $DEVICE = "help" ]
	then
		echo "To run this script:"
		echo "./test.sh <Device_Type> <Offset> <Path_to_file>"
		echo "ex. ./test.sh ABC 0 "
		exit
	fi
if [ -z "$3" ]
	then
		echo "Please provide a device type, offset, and path to the crash after \"./run.sh\""
		echo "ex. ./test.sh ABC 167 output/ABC/167/crashes/id:000000,sig:06,src:000005,op:havoc,rep:16"
		exit
	fi
if [ ! $OFFSET -ge 0 ]
	then
		echo "The header offset must be a number greater than or equal to 0"
		exit
	fi
if [ ! -d ../include/$DEVICE ]
	then
		echo "There is no ../include/$DEVICE directory"
		exit
	fi
if [ ! -f ../include/$DEVICE/boot.img ]
	then
		echo "There is no boot.img in the ../include/$DEVICE directory"
		exit
	fi

if [ ! -d out/$DEVICE ]
	then
		mkdir -p out/$DEVICE
	fi

#Detect page size of the device type
PAGE_SIZE=`od -j 36 -t x4 -N 4 -An ../include/$DEVICE/boot.img`
PAGE_SIZE=`echo "ibase=16; $PAGE_SIZE" | bc`

#Detect how many bytes the headers are supposed to be
HEADER_SIZE=$(stat --printf="%s" testcases/$DEVICE/$OFFSET/signature.bin)

out/signature_test_tool $OFFSET $HEADER_SIZE $PAGE_SIZE $DEVICE $FILE

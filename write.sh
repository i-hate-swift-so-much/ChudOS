#!/bin/bash

#the first argument should always be the dest path. e.g "/dev/sdb"

SRC_IMG="bin/ChudOS.img"

FILE=$1 

if [ ! -f $FILE ]; then
   echo "Couldn't find file ${FILE}"
   exit 1
fi

umount $FILE
sudo dd if=$SRC_IMG of=$FILE conv=fsync status=progress
sync
echo "Wrote ${SRC_IMG} to ${FILE}"
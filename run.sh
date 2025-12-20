#!/bin/bash
make
rm -rf qemu.log
qemu-system-x86_64 -drive format=raw,file=bin/ChudOS.iso -boot c -no-reboot -d int,in_asm -D qemu.log -monitor stdio -device VGA,vgamem_mb=3
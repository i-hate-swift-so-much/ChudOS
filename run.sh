#!/bin/bash
make
rm -rf qemu.log
qemu-system-x86_64 -drive format=raw,file=bin/ChudOS.iso,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -boot c -no-reboot -no-shutdown -d int,in_asm -D qemu.log -monitor stdio -device VGA,vgamem_mb=3 -machine type=pc -m 1G
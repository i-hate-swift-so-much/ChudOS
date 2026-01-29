#!/bin/bash
rm -rf qemu.log

version_patch="1"
version_minor="1"
version_major="0"

args=""

has_all=0
debug_flag_set=0
silent_flag_set=0
run_with_qemu=0
run_with_qemu_legacy_drivers=0

for arg in "$@"; do
    if [[ "$arg" == "all" ]]; then
        has_all= 1
    elif [[ "$arg" == "-s" && $silent_flag_set == 0 ]]; then
        set +x
        args+=" -s"
        silent_flag_set=1
    elif [[ "$arg" == "debug" || "$arg" == "-d" && $debug_flag_set == 0 ]]; then
        args+=" debug"
        debug_flag_set=1
    elif [[ "$arg" == "run" || "$arg" == "-r" ]]; then
        run_with_qemu=1
    elif [[ "$arg" == "run-legacy" || "$arg" == "-rl" ]]; then
        run_with_qemu_legacy_drivers=1
    fi
done

if [[ $has_all -eq 0 ]]; then
    args+=" all"
fi

echo "Making ChudOS v${version_major}.${version_minor}.${version_patch} with arguments$args"
echo "$(date)"

make $args BUILD_MAJOR=$version_major BUILD_MINOR=$version_minor BUILD_PATCH=$version_patch
if [[ $run_with_qemu_legacy_drivers == 1 ]]; then
    qemu-system-x86_64 -fda bin/ChudOS.img -boot c,strict=on -no-reboot -no-shutdown -d int,in_asm -D qemu.log -monitor stdio -device VGA,vgamem_mb=3 -machine q35 -m 1G -mem-path ./bin/junk_mem.bin
fi

if [[ $run_with_qemu == 1 ]]; then
    qemu-system-x86_64 -drive format=raw,file=bin/ChudOS.img,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -boot c,strict=on -no-reboot -no-shutdown -d int,in_asm -D qemu.log -monitor stdio -device VGA,vgamem_mb=3 -machine q35 -m 1G -mem-path ./bin/junk_mem.bin
fi
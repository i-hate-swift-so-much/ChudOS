#!/bin/bash
rm -rf qemu.log

version_patch="3"
version_minor="1"
version_major="0"

build_count_file="build.txt"

args=""

has_all=0
debug_flag_set=0
silent_flag_set=0
run_with_qemu=0
run_with_qemu_legacy_drivers=0

if [[ ! -f "$build_count_file" ]]; then
    echo 0 > "$build_count_file"
fi

current_build_count=$(<"$build_count_file")

((new_build_count = current_build_count + 1))

echo "$new_build_count" > "$build_count_file.tmp" && mv "$build_count_file.tmp" "$build_count_file"

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

make $args VERSION_MAJOR=$version_major VERSION_MINOR=$version_minor VERSION_PATCH=$version_patch BUILD=$new_build_count

echo "Built ChudOS v${version_major}.${version_minor}.${version_patch} Build ${new_build_count} with arguments$args"
echo "$(date)"

if [[ $run_with_qemu_legacy_drivers == 1 ]]; then
    qemu-system-x86_64 -fda bin/ChudOS.img -boot a,strict=on -no-reboot -no-shutdown -d int,in_asm -D qemu.log -monitor stdio -device VGA,vgamem_mb=3 -machine q35 -m 1G -mem-path ./bin/junk_mem.bin
fi

if [[ $run_with_qemu == 1 ]]; then
    qemu-system-x86_64 -drive format=raw,file=bin/ChudOS.img,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -boot c,strict=on -no-reboot -no-shutdown -d int,in_asm -D qemu.log -monitor stdio -device VGA,vgamem_mb=3 -machine q35 -m 1G -mem-path ./bin/junk_mem.bin
fi
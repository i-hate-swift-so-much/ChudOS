#!/bin/bash
rm -rf qemu.log

stty -echo

version_patch="5"
version_minor="1"
version_major="0"

build_count_file="build.txt"
build_type=1

args=""

has_all=0
debug_flag_set=0
silent_flag_set=0
run_with_qemu=0
run_with_qemu_legacy_drivers=0
isa_dma_check=0
floppy_r_check=0
floppy_w_check=0
floppy_sanity_check=0
only_run=0
gemfs_sanity_check=0

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
    elif [[ "$arg" == "run-qemu" || "$arg" == "-rq" ]]; then
        run_with_qemu=1
    elif [[ "$arg" == "run-legacy" || "$arg" == "-rl" ]]; then
        run_with_qemu_legacy_drivers=1
    elif [[ "$arg" == "isa_dma_sanity_check" || "$arg" == "-isa-dma-sc" && $isa_dma_check == 0 ]]; then
        args+=" isa_sanity"
        isa_dma_check=1
    elif [[ "$arg" == "floppy_sanity_check" || "$arg" == "-fdc-sc" && $floppy_sanity_check == 0 ]]; then
        args+=" floppy_sanity_r"
        args+=" floppy_sanity_w"
        floppy_r_check=1
        floppy_w_check=1
        floppy_sanity_check=1
    elif [[ "$arg" == "floppy_read_sanity_check" || "$arg" == "-fdc-r-sc" && $floppy_r_check == 0 ]]; then
        args+=" floppy_sanity_r"
        floppy_r_check=1
    elif [[ "$arg" == "isa_dma_sanity_check" || "$arg" == "-fdc-w-sc" && $floppy_w_check == 0 ]]; then
        args+=" floppy_sanity_w"
        floppy_w_check=1
    elif [[ "$arg" == "only_run" || "$arg" == "-or" ]]; then
        only_run=1
    elif [[ "$arg" == "gemfs_sanity_check" || "$arg" == "-gemfs-sc" && $gemfs_sanity_check == 0 ]]; then
        args+=" gemfs_sanity"
        gemfs_sanity_check=1
    elif [[ "$arg" == "test_user" || "$arg" == "-tu" ]]; then
        args+=" test_user"
    fi
done

if [[ $has_all -eq 0 ]]; then
    args+=" all"
fi

if [[ $only_run -eq 0 ]]; then
    make $args VERSION_MAJOR=$version_major VERSION_MINOR=$version_minor VERSION_PATCH=$version_patch BUILD=$new_build_count BUILD_TYPE=$build_type
fi

echo "Built ChudOS v${version_major}.${version_minor}.${version_patch}:${new_build_count} with arguments$args"
echo "$(date)"

if [[ $run_with_qemu_legacy_drivers == 1 ]]; then
    qemu-system-x86_64 \
        -drive file=bin/ChudOS.img,format=raw,if=floppy,readonly=off,cache=writethrough \
        -boot a,strict=on \
        -no-reboot -no-shutdown -d int,in_asm -D qemu.log -monitor stdio \
        -device VGA,vgamem_mb=3 \
        -machine pc \
        -m 2G 
    stty echo
    exit 0
fi

if [[ $run_with_qemu == 1 ]]; then
    qemu-system-x86_64 -drive format=raw,file=bin/ChudOS.img,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 \
        -boot c,strict=on \
        -no-reboot -no-shutdown -d int,in_asm -D qemu.log -monitor stdio \
        -device VGA,vgamem_mb=3 \
        -machine q35 \
        -m 2G
    stty echo
    exit 0
fi
.PHONY: clean debug all

VERSION_MAJOR ?= 0
VERSION_MINOR ?= 0
VERSION_PATCH ?= 1

BUILD ?= 0

as = nasm
ld = x86_64-elf-ld
c = x86_64-elf-gcc

asflags ?= -f bin
entryasflags ?= -f elf64
ldflags_kernel ?= -Tkernel64.ld
cflags ?= -m64 -I src/include -ffreestanding -nostdlib -c -mno-red-zone

output_img := bin/ChudOS.img

bootloader_objs := obj/boot0.bin obj/boot1.bin
boot0 := src/boot/boot0.s
boot1 := src/boot/boot1.s
boot2 := src/boot/boot2.s
boot0_obj := obj/boot0.bin
boot1_obj := obj/boot1.bin
boot2_obj := obj/boot2.bin

kernel_src := src/kernel/kernel.c
kernel_obj := obj/kernel.o
kernel_link := obj/kernel.elf
kernel_flat := obj/kernel.bin

kernel_entry_src := src/kernel/kernel_entry.s
kernel_entry_obj := obj/kernel_entry.o

drivers_src := src/kernel/drivers/Display/VGA.c src/kernel/Libraries/std.c src/kernel/drivers/LowLevel/IDT.c src/kernel/drivers/Devices/PS2/Keyborad.c src/kernel/drivers/Devices/PIC.c src/kernel/drivers/Userland/syscall.c src/kernel/drivers/LowLevel/interrupt_stubs.s src/kernel/Libraries/Math.c src/kernel/drivers/LowLevel/Memory.c src/kernel/drivers/ErrorHandling/KernelPanic.c src/kernel/drivers/LowLevel/Power.c src/kernel/drivers/ErrorHandling/Exceptions.c src/kernel/drivers/LowLevel/Timer.c src/kernel/drivers/Devices/Disk/AHCI.c src/kernel/drivers/Userland/Tasks.c src/kernel/drivers/PCI.c src/kernel/drivers/LowLevel/GDT.c src/kernel/Display/VGA_E.c src/kernel/drivers/Devices/Disk/Floppy
drivers_obj := obj/VGA.elf obj/std.elf obj/Keyboard.elf obj/IDT.elf obj/PIC.elf obj/syscall.elf obj/interrupt_stubs.elf obj/Math.elf obj/Memory.elf obj/KernelPanic.elf obj/Power.elf obj/Exceptions.elf obj/Timer.elf obj/AHCI.elf obj/Tasks.elf obj/PCI.elf obj/GDT.elf obj/VGA_E.elf obj/Floppy.elf
drivers_flat := obj/drivers.bin

all: clean build_boot boot_bin build_kernel link_kernel
ifneq ($(filter debug,$(MAKECMDGOALS)),)
    cflags += -DDEBUG -DVERSION_MAJOR=$(VERSION_MAJOR) -DVERSION_MINOR=$(VERSION_MINOR) -DVERSION_PATCH=$(VERSION_PATCH) -DBUILD=$(BUILD)
else
	cflags += -DVERSION_MAJOR=$(VERSION_MAJOR) -DVERSION_MINOR=$(VERSION_MINOR) -DVERSION_PATCH=$(VERSION_PATCH) -DBUILD=$(BUILD)
endif

clean:
	rm -rf bin/*
	rm -rf obj/*

build_boot: ${boot0} ${boot1}
	$(as) $(asflags) $(boot0) -o ${boot0_obj}
	$(as) $(asflags) $(boot1) -o ${boot1_obj}
	$(as) $(asflags) $(boot2) -o ${boot2_obj}

boot_bin: ${boot0_obj} ${boot1_obj}
	dd if=/dev/zero of=${output_img} bs=512 count=4096
	dd if=${boot0_obj} of=${output_img} bs=512 seek=0 conv=notrunc
	dd if=${boot1_obj} of=${output_img} bs=512 seek=2048 conv=notrunc
	dd if=${boot2_obj} of=${output_img} bs=512 seek=2068 conv=notrunc

build_kernel ${kernel_src} ${drivers_src} ${kernel_entry_src}: 
	${c} src/kernel/kernel_setup.c ${cflags} -o obj/kernel_setup.o
	${as} ${entryasflags} src/kernel/kernel_setup_entry.s -o obj/kernel_setup_entry.o

	${c} ${kernel_src} ${cflags} -o ${kernel_obj}
	${as} ${entryasflags} ${kernel_entry_src} -o ${kernel_entry_obj}
	${c} src/kernel/drivers/Display/VGA.c ${cflags} -o obj/VGA.elf
	${c} src/kernel/Libraries/std.c ${cflags} -o obj/std.elf
	${c} src/kernel/drivers/LowLevel/IDT.c ${cflags} -o obj/IDT.elf
	${c} src/kernel/drivers/Devices/PS2/Keyboard.c -mgeneral-regs-only ${cflags} -o obj/Keyboard.elf
	${c} src/kernel/drivers/Devices/PIC.c ${cflags} -o obj/PIC.elf
	${c} src/kernel/drivers/Userland/syscall.c ${cflags} -mgeneral-regs-only -o obj/syscall.elf
	${c} src/kernel/drivers/LowLevel/interrupt_stubs.s ${cflags} -mgeneral-regs-only -o obj/interrupt_stubs.elf
	${c} src/kernel/Libraries/Math.c ${cflags} -o obj/Math.elf
	${c} src/kernel/drivers/LowLevel/Memory.c ${cflags} -o obj/Memory.elf
	${c} src/kernel/drivers/LowLevel/Power.c ${cflags} -o obj/Power.elf
	${c} src/kernel/drivers/ErrorHandling/KernelPanic.c ${cflags} -o obj/KernelPanic.elf
	${c} src/kernel/drivers/ErrorHandling/Exceptions.c ${cflags} -o obj/Exceptions.elf
	${c} src/kernel/drivers/LowLevel/Timer.c ${cflags} -o obj/Timer.elf
	${c} src/kernel/drivers/Devices/Disk/AHCI.c ${cflags} -o obj/AHCI.elf
	${c} src/kernel/drivers/Userland/Tasks.c ${cflags} -o obj/Tasks.elf
	${c} src/kernel/drivers/Devices/PCI.c ${cflags} -o obj/PCI.elf
	${c} src/kernel/drivers/LowLevel/GDT.c ${cflags} -o obj/GDT.elf
	${c} src/kernel/drivers/Display/VGA_E.c ${cflags} -o obj/VGA_E.elf
	${c} src/kernel/drivers/Devices/Disk/Floppy.c ${cflags} -o obj/Floppy.elf

link_kernel ${kernel_obj} ${drivers_obj}:
	${ld} -TkernelSetup64.ld -o obj/kernel_setup.elf obj/kernel_setup_entry.o obj/kernel_setup.o
	x86_64-elf-objcopy -O binary obj/kernel_setup.elf obj/kernel_setup.bin

	${ld} ${ldflags_kernel} -o ${kernel_link} ${kernel_entry_obj} ${drivers_obj} ${kernel_obj}
	x86_64-elf-objcopy -O binary ${kernel_link} ${kernel_flat}
	dd if=${kernel_flat} of=${output_img} bs=512 seek=2448 conv=notrunc
	dd if=obj/kernel_setup.bin of=${output_img} bs=512 seek=2648 conv=notrunc
.PHONY: clean debug all

BUILD_MAJOR ?= 0
BUILD_MINOR ?= 0
BUILD_PATCH ?= 1

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

drivers_src := src/kernel/drivers/VGA.c src/kernel/lib/std.c src/kernel/drivers/IDT.c src/kernel/drivers/Keyborad.c src/kernel/drivers/PIC.c src/kernel/drivers/syscall.c src/kernel/drivers/interrupt_stubs.s src/kernel/drivers/Math.c src/kernel/drivers/Memory.c src/kernel/drivers/KernelPanic.c src/kernel/drivers/Power.c src/kernel/drivers/Exceptions.c src/kernel/drivers/Timer.c src/kernel/drivers/AHCI.c src/kernel/drivers/Tasks.c src/kernel/drivers/PCI.c src/kernel/drivers/GDT.c src/kernel/VGA_E.c
drivers_obj := obj/VGA.elf obj/std.elf obj/Keyboard.elf obj/IDT.elf obj/PIC.elf obj/syscall.elf obj/interrupt_stubs.elf obj/Math.elf obj/Memory.elf obj/KernelPanic.elf obj/Power.elf obj/Exceptions.elf obj/Timer.elf obj/AHCI.elf obj/Tasks.elf obj/PCI.elf obj/GDT.elf obj/VGA_E.elf
drivers_flat := obj/drivers.bin

all: clean build_boot boot_bin build_kernel link_kernel
ifneq ($(filter debug,$(MAKECMDGOALS)),)
    cflags += -DDEBUG -DVERSION_MAJOR=$(BUILD_MAJOR) -DVERSION_MINOR=$(BUILD_MINOR) -DVERSION_PATCH=$(BUILD_PATCH)
else
	cflags += -DVERSION_MAJOR=$(BUILD_MAJOR) -DVERSION_MINOR=$(BUILD_MINOR) -DVERSION_PATCH=$(BUILD_PATCH)
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
	dd if=${boot1_obj} of=${output_img} bs=512 seek=1 conv=notrunc
	dd if=${boot2_obj} of=${output_img} bs=512 seek=20 conv=notrunc

build_kernel ${kernel_src} ${drivers_src} ${kernel_entry_src}: 
	${c} src/kernel/kernel_setup.c ${cflags} -o obj/kernel_setup.o
	${as} ${entryasflags} src/kernel/kernel_setup_entry.s -o obj/kernel_setup_entry.o

	${c} ${kernel_src} ${cflags} -o ${kernel_obj}
	${as} ${entryasflags} ${kernel_entry_src} -o ${kernel_entry_obj}
	${c} src/kernel/drivers/VGA.c ${cflags} -o obj/VGA.elf
	${c} src/kernel/lib/std.c ${cflags} -o obj/std.elf
	${c} src/kernel/drivers/IDT.c ${cflags} -o obj/IDT.elf
	${c} src/kernel/drivers/Keyboard.c -mgeneral-regs-only ${cflags} -o obj/Keyboard.elf
	${c} src/kernel/drivers/PIC.c ${cflags} -o obj/PIC.elf
	${c} src/kernel/drivers/syscall.c ${cflags} -mgeneral-regs-only -o obj/syscall.elf
	${c} src/kernel/drivers/interrupt_stubs.s ${cflags} -mgeneral-regs-only -o obj/interrupt_stubs.elf
	${c} src/kernel/drivers/Math.c ${cflags} -o obj/Math.elf
	${c} src/kernel/drivers/Memory.c ${cflags} -o obj/Memory.elf
	${c} src/kernel/drivers/Power.c ${cflags} -o obj/Power.elf
	${c} src/kernel/drivers/KernelPanic.c ${cflags} -o obj/KernelPanic.elf
	${c} src/kernel/drivers/Exceptions.c ${cflags} -o obj/Exceptions.elf
	${c} src/kernel/drivers/Timer.c ${cflags} -o obj/Timer.elf
	${c} src/kernel/drivers/AHCI.c ${cflags} -o obj/AHCI.elf
	${c} src/kernel/drivers/Tasks.c ${cflags} -o obj/Tasks.elf
	${c} src/kernel/drivers/PCI.c ${cflags} -o obj/PCI.elf
	${c} src/kernel/drivers/GDT.c ${cflags} -o obj/GDT.elf
	${c} src/kernel/drivers/VGA_E.c ${cflags} -o obj/VGA_E.elf

link_kernel ${kernel_obj} ${drivers_obj}:
	${ld} -TkernelSetup64.ld -o obj/kernel_setup.elf obj/kernel_setup_entry.o obj/kernel_setup.o
	x86_64-elf-objcopy -O binary obj/kernel_setup.elf obj/kernel_setup.bin

	${ld} ${ldflags_kernel} -o ${kernel_link} ${kernel_entry_obj} ${drivers_obj} ${kernel_obj}
	x86_64-elf-objcopy -O binary ${kernel_link} ${kernel_flat}
	dd if=${kernel_flat} of=${output_img} bs=512 seek=400 conv=notrunc
	dd if=obj/kernel_setup.bin of=${output_img} bs=512 seek=600 conv=notrunc
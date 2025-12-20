as = nasm
ld = x86_64-elf-ld
c = x86_64-elf-gcc

asflags := -f bin 
entryasflags := -f elf64
ldflags_kernel := -Tkernel64.ld
cflags := -m64 -I src/include -ffreestanding -nostdlib -c

output_img := bin/ChudOS.iso

bootloader_objs := obj/boot0.bin obj/boot1.bin
boot0 := src/boot/boot0.s
boot1 := src/boot/boot1.s
boot2 := src/boot/boot2.s
boot0_obj := obj/boot0.bin
boot1_obj := obj/boot1.bin
boot2_obj := obj/boot2.bin

kernel_src := src/kernel/kernel.cpp
kernel_obj := obj/kernel.o
kernel_link := obj/kernel.elf
kernel_flat := obj/kernel.bin

kernel_entry_src := src/kernel/kernel_entry.s
kernel_entry_obj := obj/kernel_entry.o

drivers_src := src/kernel/drivers/VGA.cpp src/kernel/lib/std.cpp src/kernel/drivers/IDT.cpp src/kernel/drivers/Keyborad.cpp src/kernel/drivers/PIC.cpp src/kernel/drivers/syscall.cpp src/kernel/drivers/interrupt_stubs.s src/kernel/drivers/VBE.cpp src/kernel/drivers/Math.cpp src/kernel/drivers/VBE_Text.cpp src/kernel/lib/LinkedList.cpp src/kernel/drivers/Memory.cpp src/kernel/drivers/KernelPanic.cpp src/kernel/drivers/Power.cpp src/kernel/drivers/Exceptions.cpp src/kernel/drivers/Timer.cpp src/kernel/drivers/AHCI.cpp src/kernel/drivers/Tasks.cpp
drivers_obj := obj/VGA.elf obj/std.elf obj/Keyboard.elf obj/IDT.elf obj/PIC.elf obj/syscall.elf obj/interrupt_stubs.elf obj/VBE.elf obj/Math.elf obj/VBE_Text.elf obj/LinkedList.elf obj/Memory.elf obj/KernelPanic.elf obj/Power.elf obj/Exceptions.elf obj/Timer.elf obj/AHCI.elf obj/Tasks.elf
drivers_flat := obj/drivers.bin

all: clean build_boot boot_bin build_kernel link_kernel

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
	dd if=${boot2_obj} of=${output_img} bs=512 seek=6 conv=notrunc

build_kernel ${kernel_src} ${drivers_src} ${kernel_entry_src}: 
	${c} ${kernel_src} ${cflags} -o ${kernel_obj}
	${as} ${entryasflags} ${kernel_entry_src} -o ${kernel_entry_obj}
	${c} src/kernel/drivers/VGA.cpp ${cflags} -o obj/VGA.elf
	${c} src/kernel/lib/std.cpp ${cflags} -o obj/std.elf
	${c} src/kernel/drivers/IDT.cpp ${cflags} -o obj/IDT.elf
	${c} src/kernel/drivers/Keyboard.cpp -mgeneral-regs-only ${cflags} -o obj/Keyboard.elf
	${c} src/kernel/drivers/PIC.cpp ${cflags} -o obj/PIC.elf
	${c} src/kernel/drivers/syscall.cpp ${cflags} -mgeneral-regs-only -o obj/syscall.elf
	${c} src/kernel/drivers/interrupt_stubs.s ${cflags} -mgeneral-regs-only -o obj/interrupt_stubs.elf
	${c} src/kernel/drivers/VBE.cpp ${cflags} -o obj/VBE.elf
	${c} src/kernel/drivers/Math.cpp ${cflags} -o obj/Math.elf
	${c} src/kernel/drivers/VBE_Text.cpp ${cflags} -o obj/VBE_Text.elf
	${c} src/kernel/lib/LinkedList.cpp ${cflags} -o obj/LinkedList.elf
	${c} src/kernel/drivers/Memory.cpp ${cflags} -o obj/Memory.elf
	${c} src/kernel/drivers/Power.cpp ${cflags} -o obj/Power.elf
	${c} src/kernel/drivers/KernelPanic.cpp ${cflags} -o obj/KernelPanic.elf
	${c} src/kernel/drivers/Exceptions.cpp ${cflags} -o obj/Exceptions.elf
	${c} src/kernel/drivers/Timer.cpp ${cflags} -o obj/Timer.elf
	${c} src/kernel/drivers/AHCI.cpp ${cflags} -o obj/AHCI.elf
	${c} src/kernel/drivers/Tasks.cpp ${cflags} -o obj/Tasks.elf

link_kernel ${kernel_obj} ${drivers_obj}:
	${ld} ${ldflags_kernel} -o ${kernel_link} ${kernel_entry_obj} ${drivers_obj} ${kernel_obj}
	x86_64-elf-objcopy -O binary ${kernel_link} ${kernel_flat}
	dd if=${kernel_flat} of=${output_img} bs=512 seek=400 conv=notrunc
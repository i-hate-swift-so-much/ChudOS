# Changelog
## Fully Implemented Features
### Kernel
1. Interrupt Descriptor Table
2. PCI Bus Scanning
3. Page fault handler
4. Floppy disk reads/writes
5. ELF Loader
6. Multitasking
### Bootloader
1. Loads boot1.s, boot2.s and the kernel image into memory
2. Sets up simple paging
3. Jumps to kernel setup


## v0.1.0 -  Friday Jan 23rd 2026, 11h54 PST
### Changes
1. Began to use semantic version numbering.
2. Added more configuration to [makefile](makefile) and [build.sh](build.sh)
3. Got some more work done on AHCI
4. Started debugging [boot0](src/boot/boot0.s)/[boot1](src/boot/boot1.s) to work on real hardware
5. Made boot0.s smaller by getting rid of the hex converter and long messages, now prints a simple error/success code in the top left. See figure 1.
6. Created [changelog.md](changelog.md)
7. Some more work on AHCI
8. Added [kernel_setup.c](src/kernel/kernel_setup.c) which memcpys the kernel to 1MiB from where [boot1](src/boot/boot1.s) placed it
9. Renamed run.sh to [build.sh](build.sh)

| Code | Meaning |
| --- | ------ |
| SH | Sucessfully Read Boot1.s from HDD |
| SF | Sucessfully Read Boot1.s from Floppy |
| FH | Failed to read Read Boot1.s from HDD |
| FF | Failed to Read Boot1.s from Floppy |

Figure 1 - Success/Error codes for boot0.s

## v0.1.5
### Changes
1. Migrated the Kernel from being identity mapped to living in 511th entry of the PML4 table
2. Implemented ELF loader
a. Every process gets it's own PML4 table, and the 511th is memcpy'd from the Kernel's pages.
3. Started work on task switching
4. When allocating, if the memory handler cannot find the entry for a table, it will dynamically create more virtual memory.

### Todo
1. Implement floppy support and retry loops for boot1.s
2. Test [boot0](src/boot/boot0.s) on real hardware.
3. IDE driver for testing purposes and backwards compatibility.
### Known errors
1. [boot0](src/boot/boot0.s)/[boot1](src/boot/boot1.s) don't properly read from a liveboot USB on real hardware since the BIOS exposes it as a floppy and often remaps CHS
2. Dynamic memory allocation is very unstable.

## v0.1.6
### Changes
1. Extensive README.md update
2. Tasks now work.
3. Floppy driver now finished with LBA conversions.
4. Dynamic memory allocation is now stable.
5. Added write/exit syscalls.
6. Added custom file system.
7. Added more arguments to ./build.sh

### Todo
1. Implement IDE and AHCI support.
2. Add read/open/fork syscalls
# ChudOS Manual
---
ChudOS is a small hobby kernel I've been working on for the past 2 months.
This manual contains technical explanations of the OS and how it works. Please note that you need a decent grip on the principles of computer science to understand this at all (What an OS is, different CPU architectures, pointers, etc.). I hope I've written it to where if you understand that, you wont necessarily need to 
# 1.0 Specifications
## 1.1 Architecture
ChudOS runs on x86-64 Intel and AMD CPU's. It makes use of the legacy BIOS
## 1.2 RAM
ChudOS uses 4 level paging<sup>1, 2</sup> and currently supports 256 MiB of Virtual RAM, and does not check for the amount of physical RAM.
# 2.0 OS components
## 2.1 Bootloader
This project uses a non standard version of a bootloader, normally bootloaders are in 2 stages where the first loads the second and the second does complex tasks. This bootloader uses 3 stages of execution.
### 2.1.1 Bootloader Stage 1
The purpose of the first stage of the bootloader is simply to load the second stage. Since the BIOS only loads the first 512 bytes of the disk for the bootloader, and the second stage of the bootloader is a LOT more than that, it must load it.<sup>10</sup>
### 2.1.2 Bootloader Stage 2
The purpose of the second stage of the bootloader is to prepare the protected mode environment for the 3rd stage. See source 13 for explanation on protected mode.
### 2.1.3 Bootloader Stage 3
The purpose of the third stage of the bootloader is to prepare the long mode environment for the kernel and set up level 4 paging. See sources 2 and 15 for an explanation on level 4 paging and long mode.
## 2.2 Kernel
Currently, the kernel can do like 10 things. 
1. Print VGA text to the screen through VGA.h
2. Use a primitve formatter for printing (supports \n) through std.h.
3. Convert integers to char arrays in three formats (binary, decimal, and hexidecimal) through std.h.
4. Use keyboard interrupts.
5. Use timer interrupts.
6. Handle page faults and exception faults. (Exceptions.h)
7. Properly panics when an issue can't be resolved. (KernelPanic.h)
8. Set up the PIC. (PIC.h)
9. Map memory through various structs. (Memory.h)
10. Do a VERY simple syscall. (syscall.h)

I wrote this off the top of my head so be weary.
# 3.0 Roadmap
In the future I plan to finish the following files:
1. PCI.h/PCI.cpu | The PCI slots
2. AHCI.h/AHCI.cpu | The hard drive controller
3. Tasks.h/Tasks.cpp | The task manager (reports on C)
4. Timer.h/Timer.cpp | The scheduler

Along with this, I plan to create some new additions!
1. A file manager API
2. A shell (with support for .sh files)
3. Basic CLI applications based on the GNU library
4. Expanded syscalls
5. A standard library for user applications

And don't worry, the kernel will eventually be stabilized.
# 4.0 Sources
The below contain all sources used for this project.
1. “Intel® 64 and IA-32 Architectures Software Developer Manuals.” Intel, www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html.
2. “Paging ” OSDev Wiki, wiki.osdev.org/Paging.
3. "PCI" OSDev Wiki, wiki.osdev.org/PCI#The_PCI_Bus.
4. "PCI" UC Irvine, ics.uci.edu/~iharris/ics216/pci/PCI_22.pdf.
5. "NVME" OSDev Wiki, wiki.osdev.org/NVMe#Overview
6. "Memory Mapped Registers in C/C++." OSDev Wiki, https://wiki.osdev.org/Memory_mapped_registers_in_C/C++
7. "PCI Express" OSDev Wiki, wiki.osdev.org/PCI_Express.
8. "SATA" OSDev Wiki, wiki.osdev.org/SATA
9. "AHCI" OSDev Wiki, wiki.osdev.org/AHCI
10. "Project 4: Writing a Bootloader from Scratch" Carnegie Mellon University, www.cs.cmu.edu/~410-s07/p4/p4-boot.pdf.
11. "Detecting Memory (x86)" OSDev Wiki, wiki.osdev.org/Detecting_Memory_(x86)
12. "VESA Bios Extension (VBE) Core Functions Standard Version 2.0" Video Electronics Standard Association, www.phatcode.net/res/221/files/vbe20.pdf.
13. "Bootloader" Wikipeida, en.wikipedia.org/wiki/Bootloader.
14. "Protected Mode" OSDev Wiki, wiki.osdev.org/Protected_Mode
15. "Setting Up Long Mode" OSDev Wiki, wiki.osdev.org/Setting_Up_Long_Mode
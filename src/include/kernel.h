#pragma once

#include "Libraries/std.h"
#include "LowLevel/IDT.h"
#include "Devices/PIC.h"
#include "Devices/PS2/Keyboard.h"
#include "Display/VGA.h"
#include "Devices/IO.h"
#include "LowLevel/Power.h"
#include "Userland/syscall.h"
#include "LowLevel/Memory.h"
#include "LowLevel/Timer.h"
#include "ErrorHandling/KernelPanic.h"
#include "ErrorHandling/Exceptions.h"
#include "Devices/PCI.h"
#include "Devices/Disk/AHCI.h"
#include "LowLevel/GDT.h"
#include "Devices/Disk/Floppy.h"
#include "Userland/Tasks.h"
#include "filesys/gemfs.h"
#include "stdbool.h"
#include "stdint.h"

struct KERNEL_Boot_Status{
    char Sequence_Name[64];
    char Fail_Message[64];
    bool Success;
};

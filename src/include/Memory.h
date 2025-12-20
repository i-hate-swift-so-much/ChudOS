#pragma once
#include "stddef.h"
#include "stdint.h"
#include "IDT.h"
#include "std.h"
#include "VGA.h"

/*
    This header provides functions for the allocation of pages in virtual memory,
    along with various structs defining how memory should behave. Critical in the
    kernel as it has no real job without memory allocation and user programs.
*/

#define KERNEL_PAGE_PERMISSIONS 0b00000011
#define USER_PAGE_PERMISSIONS 0b00000111

// This is pratically a bitmap divided into 8 chunks.
// If a bit equals one, it's subesquent page is loaded.
// On the contrary, if a bit equals zero, it's subsequent
// page is available and can be mapped by mappages.
// Defines 2MB worth of physical memory.
// One struct takes up 512 bits of memory;
struct PhysicalMemoryTable{
    uint64_t PageBlocks[8] = {0,0,0,0,0,0,0,0};
}__attribute__((packed));

// Defines 512 PMT's, the size of one struct is 32KB.
// Defines 1 GB of Physical Memory
struct PhysicalMemoryPageDescriptor{
    PhysicalMemoryTable tables[512];
}__attribute__((packed));

// Defines 512 PMPD's, the size of one struct is 16MB.
// Defines 512 GB of physical memory
struct PhysicalMemoryPageDescriptorTable{
    PhysicalMemoryPageDescriptor descriptors[512];
}__attribute__((packed));  

struct PagePermissions{
    uint16_t flags; // directly maps to bits 0 through 7 of a page table entry
    bool Execute_Disable; // bit 63 of a page table entry, disables execution of a page
}__attribute__((packed));

// This dictates the flags which should be used for a page
// and the base address of a page.
struct PageDetails{
    PagePermissions flags;
    uint64_t virtual_address; // the virtual address of the page
    uint64_t physical_address; // the physical address of the page
}__attribute__((packed));

// All of these addresses are virtual
// 
struct HeapDesignator{
    uint64_t Heap_Bottom; // the starting virtual address of the heap
    uint64_t Heap_Data_Offset; // the starting virtual address of the heap's data
    uint64_t Heap_Size; // size of the heap in 
    uint32_t LinkedTaskPID; // the id of whatever task the heap is used by
}__attribute__((packed));

// This should only ever include the data about a task's actually code data and bss segment.
// The data about a task's heap are in HeapDesignator
struct TaskMemoryDefinition{
    uint64_t BaseVirtualAddress;
    uint64_t PageCount;
}__attribute__((packed));

struct PageEntries{
    uint16_t PML4_Entry;
    uint16_t PDPT_Entry;
    uint16_t PD_Entry;
    uint16_t PT_Entry;
    uint16_t Page_Offset;
}__attribute__((packed));

struct Task{
    uint32_t ProcessID;
    TaskMemoryDefinition MemoryData;
    HeapDesignator HeapData;
    InterruptRegisters SavedRegisters; // grabbed from "IDT.h", also used when an interrupt is triggered from userland
    uint8_t MaxTicks; // how many cycles a task can run for, tasks run with sudo have 40, normal user tasks 20, and background taks 10
    uint8_t UsedTicks; // this is so the scheduler can keep track of how many ticks the program has been running for before switching
    uint8_t ProcessState; // defines how the program is running. e.g. active, asleep, new, ready, blocked.
    uint16_t WaitingReason; // defines why the program is waiting if the state is set to that.
    uint32_t SleepCycle; // how many cycles the program has been sleeping, used so the task can request to sleep
    uint32_t RequestedSleepCycle; // the program sets this so it "sleeps"
    bool Available = false;
}__attribute__((packed));

extern Task KernelTask;
extern Task TaskManager[512];


uint8_t mem_GetBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page);

void mem_SetBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page);

void InitMem();

// Allocates pages
void* alloc_page(PageDetails page); 

void* free_page(PageDetails page); 

void* malloc(Task* TaskDetails);

void* memcpy(void* dest, const void* src, size_t n);

uint64_t* CalculatePagePhysicalEntryAddress(PageEntries* entries);

PageEntries ExtractPageEntries(uint64_t VirtualAddress);

uint64_t CalculatePageAddress(PageEntries entries);

PageDetails ParsePTE(uint64_t* PTE);

void* operator new(size_t size);
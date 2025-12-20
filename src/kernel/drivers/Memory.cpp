#include "Memory.h"

// Due to the way boot2.s is built and placed in memory, the physical address
// of PML4 is always the same
uint64_t PML4_Physical = 0x61000;

// The following are how many of each table actually exist in memory.
// These exist in case the kernel wants to support more memory in the future
#define PML4_Count 0x01 // ALWAYS set to one
#define PDPT_Count 0x01 // 1 page directory pointer table total
#define PD_Count 0x01 // 1 page directory total
#define PT_Count 0x20 // 32 page tables total

// these values are initialized to zero incase I want to add more memory
#define KERNEL_HEAP_SIZE 0 // size of the kernel heap in bytes
#define KERNEL_HEAP_OFFSET 0 // offset from the bottom of the heap

#define KERNEL_FLAGS 0b00000011
#define USER_FLAGS 0b00010111

#define KERNEL_PT_COUNT 16 // how many page tables the kernel uses

uint32_t VirtualMemorySize = 0; // the total size of the virtual memory in pages
bool mem_init = false; // used by the allocator to make sure the kernel has properly initialized virtual memory 

Task KernelTask;

PhysicalMemoryPageDescriptorTable KernelPMPDT;

void mem_SetBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page){
    uint16_t idx = Page / 64;       // which uint64_t
    uint16_t bit = Page % 64;       // which bit inside that pt
    KernelPMPDT.descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] |= (1ULL << bit);
}

void mem_ClearBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page) {
    uint16_t idx = Page / 64;
    uint16_t bit = Page % 64;
    KernelPMPDT.descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] &= ~(1ULL << bit);
}

uint8_t mem_GetBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page){
    uint16_t idx = Page / 64;
    uint16_t bit = Page % 64;

    uint8_t ret = (KernelPMPDT.descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] >> bit) & 1ULL;

    return ret;
}

void InitMem(){
    asm("cli");

    // allocate kernel memory
    for(int t = 0; t < KERNEL_PT_COUNT; t++){
        for(int p = 0; p < 512; p++){
            mem_SetBit(0, t, p);
        }
    }

    VirtualMemorySize = PT_Count * PD_Count * PDPT_Count;
    mem_init = true;

    TaskMemoryDefinition KernelMemory;
    KernelMemory.BaseVirtualAddress = 0x0;
    KernelMemory.PageCount = KERNEL_PT_COUNT*512;
    
    KernelTask.ProcessID = 0;
    KernelTask.MemoryData = KernelMemory;
    KernelTask.RequestedSleepCycle = 0;
    KernelTask.Available = false;

    asm("sti");
}

PageEntries ExtractPageEntries(uint64_t VirtualAddress){
    PageEntries extracted;

    extracted.PML4_Entry = (VirtualAddress >> 39) & 0x1FF;
    extracted.PDPT_Entry = (VirtualAddress >> 30) & 0x1FF;
    extracted.PD_Entry = (VirtualAddress >> 21) & 0x1FF;
    extracted.PT_Entry = (VirtualAddress >> 12) & 0x1FF;
    extracted.Page_Offset = VirtualAddress & 0xFFF;

    return extracted;
}

uint64_t CalculatePageAddress(PageEntries entries){
    // Mask out the entries to make sure its proper
    uint16_t PML4_Entry = entries.PML4_Entry & 0x1FF;
    uint16_t PDPT_Entry = entries.PDPT_Entry & 0x1FF;
    uint16_t PD_Entry = entries.PD_Entry & 0x1FF;
    uint16_t PT_Entry = entries.PT_Entry & 0x1FF;
    uint16_t Offset = entries.Page_Offset & 0xFFF;

    uint64_t address = (PML4_Entry << 39) | (PDPT_Entry << 30) | (PD_Entry << 21) | (PT_Entry << 12) | (Offset);
    uint64_t canonical_address = (int64_t)(address << 16) >> 16;
    return canonical_address;
}

// Gets the entry address of a page for setting a virtual addresses data.
// Does not account for large pages as those are unuseed by this OS.
uint64_t* CalculatePagePhysicalEntryAddress(PageEntries* entries){
    if(entries->PML4_Entry > 0 || entries->PDPT_Entry > 0 || entries->PD_Entry > 128 || entries->PT_Entry > 512){ return nullptr; };
    uint64_t* PML4_Pointer = reinterpret_cast<uint64_t*>(PML4_Physical + (entries->PML4_Entry * 8));
    uint64_t* PDPT_Pointer = reinterpret_cast<uint64_t*>((*PML4_Pointer & 0xFFFFFFFFF000ULL) + (entries->PDPT_Entry * 8));
    uint64_t* PD_Pointer = reinterpret_cast<uint64_t*>((*PDPT_Pointer & 0xFFFFFFFFF000ULL) + (entries->PD_Entry * 8));
    uint64_t* PT_Pointer = reinterpret_cast<uint64_t*>((*PD_Pointer & 0xFFFFFFFFF000ULL) + (entries->PT_Entry * 8));

    return PT_Pointer;
}

// Allocates a single page, includes protection against memory leaks
void* alloc_page(PageDetails page){
    PageEntries Deconstructed = ExtractPageEntries(page.virtual_address);
    uint16_t PDPT_Entry = Deconstructed.PDPT_Entry;
    uint16_t PD_Entry = Deconstructed.PD_Entry;
    uint16_t Page = Deconstructed.PT_Entry;

    // if the requested page exceeds the bounds of the VirtualMemory, then
    // the page cannot be allocated
    uint32_t PageEntry = Page * PD_Entry;
    if(PageEntry+2 > VirtualMemorySize){ return nullptr; } 

    PageEntries DeconstructedP = ExtractPageEntries(page.physical_address);
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    uint8_t page_bit = mem_GetBit(PDPT_EntryP, PD_EntryP, PageP); // makes sure the page is actually available
    if(page_bit == 0){
        // the physical address of the page entry we want to set
        uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);
        
        if(Page_Entry == nullptr){ afstd::printf("No page entry\n"); return nullptr; }

        uint64_t new_entry = 
            (page.physical_address) |
            (page.flags.flags) |
            (page.flags.Execute_Disable << 63);

        
        *Page_Entry = new_entry;

        // must flush the TLB with invlpg, otherwise CPU wont know the page was updated

        asm volatile("invlpg (%0)" :: "r"(static_cast<uintptr_t>(page.virtual_address)) : "memory");
    }

    PageEntries physicalData = ExtractPageEntries(page.physical_address);

    mem_SetBit(physicalData.PDPT_Entry, physicalData.PD_Entry, physicalData.PT_Entry);

    return reinterpret_cast<void*>(page.virtual_address);
}; 

// Gets rid of a single page, includes protection against memory leaks
void* free_page(PageDetails page){
    PageEntries Deconstructed = ExtractPageEntries(page.virtual_address);
    uint16_t PDPT_Entry = Deconstructed.PDPT_Entry;
    uint16_t PD_Entry = Deconstructed.PD_Entry;
    uint16_t Page = Deconstructed.PT_Entry;

    // if the requested page exceeds the bounds of the VirtualMemory, then
    // the page cannot be allocated
    uint32_t PageEntry = Page * PD_Entry;
    if(PageEntry+2 > VirtualMemorySize){ return nullptr; } 

    PageEntries DeconstructedP = ExtractPageEntries(page.physical_address);
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    uint8_t page_bit = mem_GetBit(PDPT_EntryP, PD_EntryP, PageP); // makes sure the page is actually available
    if(page_bit == 1){
        // the physical address of the page entry we want to set
        uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);
        if(!Page_Entry){ return nullptr; }
        
        uint64_t new_entry = (uint64_t)0x0000;

        *Page_Entry = new_entry;

        // must flush the TLB with invlpg, otherwise CPU wont know the page was updated
        asm volatile("invlpg (%0)" :: "r"(page.virtual_address) : "memory");
    }
    
    PageEntries physicalData = ExtractPageEntries(page.physical_address);

    mem_ClearBit(physicalData.PDPT_Entry, physicalData.PD_Entry, physicalData.PT_Entry);

    return reinterpret_cast<void*>(page.virtual_address);
}; 

PageEntries FindNextFreePhysical(){
    PageEntries ret;
    for(int PDPT_Entry = 0; PDPT_Entry < 512; PDPT_Entry++){
        for(int PD_Entry = 0; PD_Entry < 512; PD_Entry++){
            for(int PT_Entry = 0; PT_Entry < 512; PT_Entry++){
                uint16_t bit = mem_GetBit(PDPT_Entry, PD_Entry, PT_Entry);
                if(bit == 0){
                    ret.PDPT_Entry = PDPT_Entry;
                    ret.PD_Entry = PD_Entry;
                    ret.PT_Entry = PT_Entry;
                    
                    return ret;
                }
            }
        }
    }
    return ret;
}

// Allocates one page based on a task
void* malloc(Task* TaskDetails){
    uint64_t VirtualBase = TaskDetails->MemoryData.BaseVirtualAddress;
    uint64_t VirtualTop = VirtualBase + (TaskDetails->MemoryData.PageCount * 4096);

    uint64_t NextPageAddress = VirtualTop+4096;
    PageDetails NextPage;
    if (TaskDetails->ProcessID == 0){
        NextPage.flags.flags = KERNEL_FLAGS;
    }else{
        NextPage.flags.flags = USER_FLAGS;
    }
    NextPage.flags.Execute_Disable = false;
    PageEntries nextFree = FindNextFreePhysical();
    if(nextFree.PML4_Entry == 0xFF){ return nullptr; }
    NextPage.physical_address = CalculatePageAddress(nextFree);
    
    NextPage.virtual_address = NextPageAddress;

    void* allocated = alloc_page(NextPage);

    if(allocated != nullptr) { TaskDetails->MemoryData.PageCount += 1; }

    return allocated;
}


/*
    void* dest: Where to load the memory from src
    const void* src: Where the memory loaded into dest comes from
    size_t n: How many bytes should be loaded from src to dest
*/
void* memcpy(void* dest, const void* src, size_t n){
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    while(n--){
        *d++ = *s++;
    }

    return dest;
}

void* operator new(size_t size){
    int temp = 0;
    void* ptr;
    ptr = &temp;
    return ptr;
}

PageDetails ParsePTE(uint64_t* PTE){
    PageDetails ret;
    
    if(PTE == nullptr){ return ret; }

    uint64_t PhysicalAddress = (*PTE & 0xFFFFFFFFF000ULL);
    uint16_t flags = *PTE & 0x1FF;
    bool XD = *PTE >> 63;
    uint64_t VirtualAddress = reinterpret_cast<uintptr_t>(PTE);
    VirtualAddress-=PML4_Physical;

    ret.virtual_address = VirtualAddress;
    ret.physical_address = PhysicalAddress;
    ret.flags.flags = flags;
    ret.flags.Execute_Disable = XD;

    return ret;
}
#include "LowLevel/Memory.h"
#include "stdbool.h"
#include "Libraries/Math.h"

// The physical address of the start of the PML4 table, loaded by init_mem from CR3
uint64_t Kernel_PML4_Physical = 0;
uint64_t PML4_Physical = 0;

// The following are how many of each table actually exist in memory.
// These exist in case the kernel wants to support more memory in the future
#define PML4_Count 0x01 // ALWAYS set to one
#define PDPT_Count 0x01 // 1 page directory pointer table total
#define PD_Count 0x01 // 1 page directory total
#define PT_Count 0x20 // 32 page tables total

#define KERNEL_PT_COUNT 32 // how many page tables the kernel uses

uint32_t VirtualMemorySize = 0; // the total size of the virtual memory in pages
bool mem_init = false; // used by the allocator to make sure the kernel has properly initialized virtual memory 

Task* KernelTask;

PhysicalMemoryMapLevel4 KernelPMML4;

// set by Enumerate_E820
uint64_t available_mem;
uint64_t total_mem;

uint64_t PhysicalPagesUsed = 0;

// Set a bit from the bitmap of physical space, used for fragmentation
void mem_SetBit(uint16_t PageMapLevel4, uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page){
    uint16_t idx = Page / 64;       // which uint64_t
    uint16_t bit = Page % 64;       // which bit inside that pt
    KernelPMML4.pagedescriptors[PageMapLevel4].descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] |= (1ULL << bit);
    PhysicalPagesUsed++;
}

// Clear a bit from the bitmap of physical space, used for fragmentation
void mem_ClearBit(uint16_t PageMapLevel4, uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page) {
    uint16_t idx = Page / 64;
    uint16_t bit = Page % 64;
    KernelPMML4.pagedescriptors[PageMapLevel4].descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] &= (0ULL << bit);
    PhysicalPagesUsed--;
}

// Get a bit from the bitmap of physical space, used for fragmentation
uint8_t mem_GetBit(uint16_t PageMapLevel4, uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page){
    uint16_t idx = Page / 64;
    uint16_t bit = Page % 64;

    uint8_t ret = (KernelPMML4.pagedescriptors[PageMapLevel4].descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] >> bit) & 1ULL;

    return ret;
}

// Sets up a new PML4 table for a user task. Follows the basic code from boot2.s. Returns the new PML4 tables physical address
uint64_t Create_User_Memory(){
    mem_set_cr3(Kernel_PML4_Physical);

    void* pml4 = malloc(KernelTask);

    uint64_t* Kernel_PML4_Virtual = (uint64_t*)(Kernel_PML4_Physical + 0xffff800000000000ULL);

    memcpy((void*)pml4, (void*)Kernel_PML4_Virtual, 0x1000);

    return phys_addr(pml4);
}

// syncs PML4_Physical to CR3
void mem_sync_cr3(){
    // set PML4_Physical
    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (PML4_Physical)
        : : "%rax"
    );
}

void mem_set_cr3(uint64_t addr){
    // set the CR3 register to the new PML4
    asm volatile(
        "cli\n"
        "movq %0, %%cr3\n"
        : : "r" (addr) :
    );
    mem_sync_cr3();
    asm volatile("sti");
}

// Initialize virtual memory, only ever called at kernel boot
void InitMem(){
    asm("cli");

    mem_sync_cr3();

    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (Kernel_PML4_Physical)
        : : "%rax"
    );

    // zero out the identity mapped kernel, which is no longer used after kernel_setup
    memset((void*) Kernel_PML4_Physical, 0, 0x800);

    // zero out the PMPDT
    memset(&KernelPMML4, 0, sizeof(KernelPMML4));
    
    // allocate kernel memory
    for(int t = 0; t < KERNEL_PT_COUNT; t++){
        for(int p = 0; p < 512; p++){
            mem_SetBit(0, 0, t, p);
        }
    }

    memset(TaskManager, 0, sizeof(TaskManager));
    
    VirtualMemorySize = PT_Count * PD_Count * PDPT_Count;
    mem_init = true;

    TaskMemoryDefinition KernelMemory;
    KernelMemory.BaseVirtualAddress = 0xffff800000000000;
    KernelMemory.PageCount = 512;
    
    KernelTask = (Task*)&TaskManager[0];

    KernelTask->ProcessID = 0;
    KernelTask->MemoryData = KernelMemory;
    KernelTask->RequestedSleepCycle = 0;
    KernelTask->Exists = true;

    uint64_t mem = Create_User_Memory();

    mem_set_cr3(mem);

    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (Kernel_PML4_Physical)
        : : "%rax"
    );

    asm("sti");
}

// Convert a virtual address into a PageEntries struct
PageEntries ExtractPageEntries(uint64_t VirtualAddress){
    PageEntries extracted;

    extracted.PML4_Entry = (VirtualAddress >> 39) & 0x1FF;
    extracted.PDPT_Entry = (VirtualAddress >> 30) & 0x1FF;
    extracted.PD_Entry = (VirtualAddress >> 21) & 0x1FF;
    extracted.PT_Entry = (VirtualAddress >> 12) & 0x1FF;
    extracted.Page_Offset = VirtualAddress & 0xFFF;

    return extracted;
}

// Convert a PageEntries struct to a single
// virtual address.
uint64_t CalculatePageAddress(PageEntries* entries){
    // Mask out the entries to make sure its proper
    uint16_t PML4_Entry = entries->PML4_Entry & 0x1FF;
    uint16_t PDPT_Entry = entries->PDPT_Entry & 0x1FF;
    uint16_t PD_Entry = entries->PD_Entry & 0x1FF;
    uint16_t PT_Entry = entries->PT_Entry & 0x1FF;
    uint16_t Offset = entries->Page_Offset & 0xFFF;

    uint64_t address = ((uint64_t)PML4_Entry << 39) | ((uint64_t)PDPT_Entry << 30) | ((uint64_t)PD_Entry << 21) | ((uint64_t)PT_Entry << 12) | ((uint64_t)Offset);
    uint64_t canonical_address = (int64_t)(address << 16) >> 16;
    return canonical_address;
}

/**
 * @brief Flushes the entire TLB, used when modifying high level entries.
 */
void FlushTLB(){
    asm volatile(
        "movq %cr3, %rax\n"
        "movq %rax, %cr3\n"
    );
}

/**
 * @brief Used by CalculatePagePhysicalEntryAddress incase a entry isn't set.
 * @param e_addr The virtual address of the entry.
 * @param flags The flags for the new entry.
 * @param flush Whether or not a full TLB flush should occur if a new entry is made.
 * @param granularity Only used for debugging, shows which level of the PML4 it is.
 */
void EnsureEntry(uint64_t* e_addr, uint8_t flags, bool flush, int granularity){
    uint64_t fix = (*e_addr) & 0xFFFFFFFFFFFF000ULL;
    if(fix == 0){
        void* new = malloc(KernelTask);
        
        memset(new, 0, 0x1000);

        *e_addr = (uint64_t)(
            phys_addr(new) |
            flags
        );
        if(flush){
            FlushTLB();
        }
    }
}

/**
 * @brief Gets the entry address of a page. If a certain part doesn't exist, it will allocate a new structure. 
 */
uint64_t* CalculatePagePhysicalEntryAddress(PageEntries* entries){
    uint16_t flags = KERNEL_FLAGS;
    if(entries->PML4_Entry < 256){
        flags = USER_FLAGS;
    }
    uint64_t* PML4_Pointer = (uint64_t*)((PML4_Physical + (entries->PML4_Entry * 8)) + 0xffff800000000000ULL);
    
    EnsureEntry(PML4_Pointer, flags, true, 3);

    uint64_t* PDPT_Pointer = (uint64_t*)(((*PML4_Pointer & 0xFFFFFFFFF000ULL) + (entries->PDPT_Entry * 8)) + 0xffff800000000000ULL);

    EnsureEntry(PDPT_Pointer, flags, true, 2);

    uint64_t* PD_Pointer = (uint64_t*)(((*PDPT_Pointer & 0xFFFFFFFFF000ULL) + (entries->PD_Entry * 8)) + 0xffff800000000000ULL);

    EnsureEntry(PD_Pointer, flags, true, 1);

    uint64_t* PT_Pointer = (uint64_t*)(((*PD_Pointer & 0xFFFFFFFFF000ULL) + (entries->PT_Entry * 8)) + 0xffff800000000000ULL);


    return PT_Pointer;
}

// Allocates a single page, includes protection against memory leaks
void* alloc_page(PageDetails* page){
    PageEntries Deconstructed = ExtractPageEntries(page->virtual_address);
    uint16_t PDPT_Entry = Deconstructed.PDPT_Entry;
    uint16_t PD_Entry = Deconstructed.PD_Entry;
    uint16_t Page = Deconstructed.PT_Entry;

    PageEntries DeconstructedP = ExtractPageEntries(page->physical_address);
    uint16_t PML4_EntryP = DeconstructedP.PML4_Entry;
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    uint8_t page_bit = mem_GetBit(PML4_EntryP, PDPT_EntryP, PD_EntryP, PageP); // makes sure the page is actually available
    
    if(page_bit == 0){
        // the physical address of the page entry we want to set
        uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);

        if(Page_Entry == NULL){ printf("No page entry\n"); return NULL; }

        uint64_t new_entry = 
            (page->physical_address) |
            (page->flags.flags) |
            (((uint64_t)page->flags.Execute_Disable) << 63);

        *Page_Entry = new_entry;

        // must flush the TLB with invlpg, otherwise CPU wont know the page was updated

        asm volatile("invlpg (%0)" :: "r"((uintptr_t)(page->virtual_address)) : "memory");
    }else{
        print("Page unavailable\n", 0);
        return (void*)(page->virtual_address);
    }

    PageEntries physicalData = ExtractPageEntries(page->physical_address);

    mem_SetBit(physicalData.PML4_Entry, physicalData.PDPT_Entry, physicalData.PD_Entry, physicalData.PT_Entry);

    return (void*)(page->virtual_address);
}; 

// Gets rid of a single page, includes protection against memory leaks
void* free_page(PageDetails* page){
    PageEntries Deconstructed = ExtractPageEntries(page->virtual_address);
    uint16_t PDPT_Entry = Deconstructed.PDPT_Entry;
    uint16_t PD_Entry = Deconstructed.PD_Entry;
    uint16_t Page = Deconstructed.PT_Entry;

    // if the requested page exceeds the bounds of the VirtualMemory, then
    // the page cannot be allocated
    uint32_t PageEntry = Page * PD_Entry;
    if(PageEntry+2 > VirtualMemorySize){ return NULL; } 

    PageEntries DeconstructedP = ExtractPageEntries(page->physical_address);
    uint16_t PML4_EntryP = DeconstructedP.PML4_Entry;
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    uint8_t page_bit = mem_GetBit(PML4_EntryP, PDPT_EntryP, PD_EntryP, PageP); // makes sure the page is actually available
    if(page_bit == 1){
        // the physical address of the page entry we want to set
        uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);
        if(!Page_Entry){ return NULL; }
        
        uint64_t new_entry = (uint64_t)0x0000;

        *Page_Entry = new_entry;

        // must flush the TLB with invlpg, otherwise CPU wont know the page was updated
        asm volatile("invlpg (%0)" :: "r"(page->virtual_address) : "memory");
    }
    
    PageEntries physicalData = ExtractPageEntries(page->physical_address);

    mem_ClearBit(physicalData.PML4_Entry, physicalData.PDPT_Entry, physicalData.PD_Entry, physicalData.PT_Entry);

    return (void*)(page->virtual_address);
}; 

PageEntries FindNextFreePhysical(){
    PageEntries ret;
    for(int PML4_Entry = 0; PML4_Entry < 512; PML4_Entry++){
        for(int PDPT_Entry = 0; PDPT_Entry < 512; PDPT_Entry++){
            for(int PD_Entry = 0; PD_Entry < 512; PD_Entry++){
                for(int PT_Entry = 0; PT_Entry < 512; PT_Entry++){
                    uint16_t bit = mem_GetBit(PML4_Entry, PDPT_Entry, PD_Entry, PT_Entry);
                   if(bit == 0){
                        ret.PML4_Entry = PML4_Entry;
                        ret.PDPT_Entry = PDPT_Entry;
                        ret.PD_Entry = PD_Entry;
                        ret.PT_Entry = PT_Entry;
                        ret.Page_Offset = 0;

                        return ret;
                    }
                }
            }
        }
    }
    
    ret.PML4_Entry = 512;
    ret.PDPT_Entry = 512;
    ret.PD_Entry = 512;
    ret.PT_Entry = 512;
    ret.Page_Offset = 0;
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
    if(nextFree.PML4_Entry == 0xFF){ return NULL; }
    NextPage.physical_address = CalculatePageAddress(&nextFree);
    
    NextPage.virtual_address = NextPageAddress;

    void* allocated = alloc_page(&NextPage);

    if(allocated != NULL) { TaskDetails->MemoryData.PageCount += 1; }

    return allocated;
}

/*
    Normally used for BARs, this function maps a page to a
    tasks memory, but you can choose which physical address
    you want and whatever flags you want.
*/
void* malloc_specific(Task* TaskDetails, uint64_t RequestedAddress, PagePermissions* permissions){
    uint64_t VirtualBase = TaskDetails->MemoryData.BaseVirtualAddress;
    uint64_t VirtualTop = VirtualBase + (TaskDetails->MemoryData.PageCount * 0x1000);

    uint64_t NextPageAddress = VirtualTop+0x1000;
    PageDetails NextPage;
    NextPage.flags.flags = permissions->flags;
    NextPage.flags.Execute_Disable = permissions->Execute_Disable;
    NextPage.physical_address = RequestedAddress;
    
    NextPage.virtual_address = NextPageAddress;

    void* allocated = alloc_page(&NextPage);

    if(allocated != NULL) { TaskDetails->MemoryData.PageCount += 1; }

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

void* memcpyw(void* dest, const void* src, size_t n){
    uint16_t* d = (uint16_t*)dest;
    const uint16_t* s = (const uint16_t*)src;

    while(n-=2){
        *(d+=2) = *(s+=2);
    }

    return dest;
}

void memsetw(void* dest, uint16_t value, size_t bytes){
    uint16_t* dest_m = (uint16_t*) dest;
    
    while(bytes-=2){
        *(dest_m+=2) = value;
    }
}

PageDetails ParsePTE(uint64_t* PTE){
    PageDetails ret;
    
    if(PTE == NULL){ return ret; }

    uint64_t PhysicalAddress = (*PTE & 0xFFFFFFFFF000ULL);
    uint16_t flags = *PTE & 0x1FF;
    bool XD = *PTE >> 63;
    uint64_t VirtualAddress = (uintptr_t)(PTE);
    VirtualAddress-=PML4_Physical;

    ret.virtual_address = VirtualAddress;
    ret.physical_address = PhysicalAddress;
    ret.flags.flags = flags;
    ret.flags.Execute_Disable = XD;

    return ret;
}

void mem_bitmap_dump(uint16_t PT){
    cls();
    char cur;
    for(int y = 0; y < 16; y++){
        for(int x = 0; x < 32; x++){
            if(mem_GetBit(0, 0, PT, x+(y*32)) == 0){ cur = '0'; } else {cur = '1'; }
            WriteCharacter(cur, x, y);
            setCursor(0, 16);
        }
    }
}

void memset(void* dest, uint8_t value, size_t bytes){
    uint8_t* dest_m = (uint8_t*) dest;
    
    while(bytes--){
        *dest_m++ = value;
    }
}

void PrintMemorySize(size_t bytes){
    if(bytes == 0){ 
        print("0 Byte(s)", 0);
    }
    
    uint64_t kib = bytes / 0x400; // kibibyte count
    uint64_t mib = bytes / 0x100000; // mibibyte count
    uint64_t gib = bytes / 0x40000000; // gibibyte count

    char print[24];

    if(gib){
        int_to_char_array(gib, print, sizeof(print), 0);
        print_debug(print, 0);
        print_debug(" GiB(s)", 0);
    }else if(mib){
        int_to_char_array(mib, print, sizeof(print), 0);
        print_debug(print, 0);
        print_debug(" MiB(s)", 0);
    }else if(kib){
        int_to_char_array(kib, print, sizeof(print), 0);
        print_debug(print, 0);
        print_debug(" KiB(s)", 0);
    }else{
        int_to_char_array(bytes, print, sizeof(print), 0);
        print_debug(print, 0);
        print_debug(" Byte(s)", 0);
    }
}

void E820_Dump_Descriptor(E820_Range_Descriptor* descriptor){
    char test_char[36];
    int_to_char_array_hex(abs(descriptor->Base_Address), test_char, sizeof(test_char), 10);
    print_debug(test_char, 0);
    print_debug("->", 0);
    int_to_char_array_hex(abs(descriptor->Length), test_char, sizeof(test_char), 10);
    print_debug(test_char, 0);
    print_debug(" (", 0);
    int_to_char_array(abs(descriptor->Type), test_char, sizeof(test_char), 0);
    print_debug(test_char, 0);
    print_debug(")   ", 0);
    PrintMemorySize(descriptor->Length);
    if(descriptor->Type == 1){
        print_debug(" of available memory\n", 0);
    }else{
        print_debug(" of unavailable memory\n", 0);
    }
}

void Enumerate_E820(){
    uintptr_t e820_ptr = (uintptr_t)E820_BUFFER_ADDRESS;
    E820_Range_Descriptor* cur_descriptor = (E820_Range_Descriptor*)e820_ptr;

    uintptr_t loop_count_ptr = (uintptr_t)(E820_BUFFER_ADDRESS-24);
    uint32_t loop_count = (*(uint32_t*)loop_count_ptr) / 24;

    #ifdef DEBUG
        char test_char[36];
        int_to_char_array(loop_count, test_char, sizeof(test_char), 0);
        print("\n", 0);
        print(test_char, 0);
        print(" memory regions detected\n", 0);
        print_debug("BASE          LIMIT        TYPE  SIZE\n",0);
    #endif

    for(int i = 0; i < loop_count-1; i++){
        #ifdef DEBUG
            E820_Dump_Descriptor(cur_descriptor);
        #endif
        if(cur_descriptor->Type == 1){
            available_mem+=cur_descriptor->Length;
        }else{
            // loop through all the pages covered by the limit,
            // since they are reserved we just set them to used
            // but keep them unmapped.
            uint64_t cur_base = cur_descriptor->Base_Address;
            for(int i = 0; i < cur_descriptor->Length; i+=0x1000){
                PageEntries entries = ExtractPageEntries(cur_base);
                mem_SetBit(entries.PML4_Entry, entries.PDPT_Entry, entries.PD_Entry, entries.PT_Entry);
                cur_base+=0x1000;
            }
            
        }
        total_mem+=cur_descriptor->Length;

        cur_descriptor++;
    }
    #ifdef DEBUG
        SetTextColor(LCYAN, BLACK);
        print_debug("Available memory: ", 0);
        PrintMemorySize(available_mem);
        print_debug("\nUnavailable memory: ", 0);
        PrintMemorySize(total_mem-available_mem);
        print_debug("\nTotal Memory: ", 0);
        PrintMemorySize(total_mem);
        print("\n", 0);
    #endif
}

/**
 * @brief Returns the physical address of a virtual address. Does account for the offset
 * @param pointer Pointer to the virtual address you want to convert to physical
 */
uint64_t phys_addr(void* pointer){
    uint64_t addr = (uint64_t)pointer & ~0xFFF; // convert the pointer to it's address and get rid of the offset
    uint16_t offset = (uint64_t)pointer & 0xFFF; // get the offset of the pointer

    PageEntries entries = ExtractPageEntries(addr);

    uint64_t* info = CalculatePagePhysicalEntryAddress(&entries);

    return (*info & 0xFFFFFFFFFFFFF000) | offset;
}

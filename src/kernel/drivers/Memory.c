#include "Memory.h"
#include "stdbool.h"
#include "Math.h"

// Due to the way boot2.s is built and placed in memory, the physical address
// of PML4 is always the same
uint64_t PML4_Physical = 0x61000;

// The following are how many of each table actually exist in memory.
// These exist in case the kernel wants to support more memory in the future
#define PML4_Count 0x01 // ALWAYS set to one
#define PDPT_Count 0x01 // 1 page directory pointer table total
#define PD_Count 0x01 // 1 page directory total
#define PT_Count 0x20 // 32 page tables total

#define KERNEL_PT_COUNT 24 // how many page tables the kernel uses

uint32_t VirtualMemorySize = 0; // the total size of the virtual memory in pages
bool mem_init = false; // used by the allocator to make sure the kernel has properly initialized virtual memory 

Task KernelTask;

uintptr_t KernelPMPDT_Address = 0x1000000;

PhysicalMemoryPageDescriptorTable KernelPMPDT;

// set by Enumerate_E820
uint64_t available_mem;
uint64_t total_mem;

// Set a bit from the bitmap of physical space, used for fragmentation
void mem_SetBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page){
    uint16_t idx = Page / 64;       // which uint64_t
    uint16_t bit = Page % 64;       // which bit inside that pt
    KernelPMPDT.descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] |= (1ULL << bit);
}

// Zero a bit from the bitmap of physical space, used for fragmentation
void mem_ClearBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page) {
    uint16_t idx = Page / 64;
    uint16_t bit = Page % 64;
    KernelPMPDT.descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] &= (0ULL << bit);
}

// Get a bit from the bitmap of physical space, used for fragmentation
uint8_t mem_GetBit(uint16_t PageDescriptorTable, uint16_t PageTable, uint16_t Page){
    uint16_t idx = Page / 64;
    uint16_t bit = Page % 64;

    uint8_t ret = (KernelPMPDT.descriptors[PageDescriptorTable].tables[PageTable].PageBlocks[idx] >> bit) & 1ULL;

    return ret;
}

// Initialize virtual memory, only ever called at kernel boot
void InitMem(){
    asm("cli");

    // zero out the PMPDT
    memset(&KernelPMPDT, 0, sizeof(KernelPMPDT));

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
uint64_t CalculatePageAddress(PageEntries entries){
    // Mask out the entries to make sure its proper
    uint16_t PML4_Entry = entries.PML4_Entry & 0x1FF;
    uint16_t PDPT_Entry = entries.PDPT_Entry & 0x1FF;
    uint16_t PD_Entry = entries.PD_Entry & 0x1FF;
    uint16_t PT_Entry = entries.PT_Entry & 0x1FF;
    uint16_t Offset = entries.Page_Offset & 0xFFF;

    uint64_t address = ((uint64_t)PML4_Entry << 39) | ((uint64_t)PDPT_Entry << 30) | ((uint64_t)PD_Entry << 21) | ((uint64_t)PT_Entry << 12) | ((uint64_t)Offset);
    uint64_t canonical_address = (int64_t)(address << 16) >> 16;
    return canonical_address;
}

// Gets the entry address of a page for setting a virtual addresses data.
// Does not account for large pages as those are unuseed by this OS.
uint64_t* CalculatePagePhysicalEntryAddress(PageEntries* entries){
    uint64_t* PML4_Pointer = (uint64_t*)(PML4_Physical + (entries->PML4_Entry * 8));
    uint64_t* PDPT_Pointer = (uint64_t*)((*PML4_Pointer & 0xFFFFFFFFF000ULL) + (entries->PDPT_Entry * 8));
    if(PDPT_Pointer == NULL){ return PML4_Pointer; }
    uint64_t* PD_Pointer = (uint64_t*)((*PDPT_Pointer & 0xFFFFFFFFF000ULL) + (entries->PD_Entry * 8));
    if(PD_Pointer == NULL){ return PDPT_Pointer; }
    uint64_t* PT_Pointer = (uint64_t*)((*PD_Pointer & 0xFFFFFFFFF000ULL) + (entries->PT_Entry * 8));
    if(PT_Pointer == NULL){ return PD_Pointer; }

    return PT_Pointer;
}

// Allocates a single page, includes protection against memory leaks
void* alloc_page(PageDetails page){
    PageEntries Deconstructed = ExtractPageEntries(page.virtual_address);
    uint16_t PDPT_Entry = Deconstructed.PDPT_Entry;
    uint16_t PD_Entry = Deconstructed.PD_Entry;
    uint16_t Page = Deconstructed.PT_Entry;

    PageEntries DeconstructedP = ExtractPageEntries(page.physical_address);
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    uint8_t page_bit = mem_GetBit(PDPT_EntryP, PD_EntryP, PageP); // makes sure the page is actually available
    if(page_bit == 0){
        // the physical address of the page entry we want to set
        uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);
        
        if(Page_Entry == NULL){ printf("No page entry\n", 0); return NULL; }

        uint64_t new_entry = 
            (page.physical_address) |
            (page.flags.flags) |
            (((uint64_t)page.flags.Execute_Disable) << 63);


        *Page_Entry = new_entry;

        // must flush the TLB with invlpg, otherwise CPU wont know the page was updated

        asm volatile("invlpg (%0)" :: "r"((uintptr_t)(page.virtual_address)) : "memory");
    }else{
        printf("Page unavailable\n", 0);
        return (void*)(page.virtual_address);
    }

    PageEntries physicalData = ExtractPageEntries(page.physical_address);

    mem_SetBit(physicalData.PDPT_Entry, physicalData.PD_Entry, physicalData.PT_Entry);

    return (void*)(page.virtual_address);
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
    if(PageEntry+2 > VirtualMemorySize){ return NULL; } 

    PageEntries DeconstructedP = ExtractPageEntries(page.physical_address);
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    uint8_t page_bit = mem_GetBit(PDPT_EntryP, PD_EntryP, PageP); // makes sure the page is actually available
    if(page_bit == 1){
        // the physical address of the page entry we want to set
        uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);
        if(!Page_Entry){ return NULL; }
        
        uint64_t new_entry = (uint64_t)0x0000;

        *Page_Entry = new_entry;

        // must flush the TLB with invlpg, otherwise CPU wont know the page was updated
        asm volatile("invlpg (%0)" :: "r"(page.virtual_address) : "memory");
    }
    
    PageEntries physicalData = ExtractPageEntries(page.physical_address);

    mem_ClearBit(physicalData.PDPT_Entry, physicalData.PD_Entry, physicalData.PT_Entry);

    return (void*)(page.virtual_address);
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
    if(nextFree.PML4_Entry == 0xFF){ return NULL; }
    NextPage.physical_address = CalculatePageAddress(nextFree);
    
    NextPage.virtual_address = NextPageAddress;

    void* allocated = alloc_page(NextPage);

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

    void* allocated = alloc_page(NextPage);

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

// Finds an entry in the Page Directory Pointer Table that isn't already present.
// Used by void create_pd(int pages);
void* find_free_pdpt(){
    uint64_t* cur_pde;
    PageEntries cur_target;

    bool present = false;

    uint16_t pdpt;
    uint16_t pml4;

    while(!present && pml4 > 512){
        if(*cur_pde & 1){present = true;}

        cur_target.PDPT_Entry = pdpt;
        cur_target.PML4_Entry = pml4;
        
        cur_pde = CalculatePagePhysicalEntryAddress(&cur_target);

        pdpt++;
        if(pdpt == 512){ pdpt = 0; pml4++; }
    }
    if(pml4 >= 512){ printf("Out of memory!\n", 0); return NULL;}
}

void mem_bitmap_dump(uint16_t PT){
    cls();
    char cur;
    for(int y = 0; y < 16; y++){
        for(int x = 0; x < 32; x++){
            if(mem_GetBit(0, PT, x+(y*32)) == 0){ cur = '0'; } else {cur = '1'; }
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

// Finds an entry in the PML4 that isn't already present.
// Used by void create_pd(int pages);
void* find_free_pml4(){
    uint64_t* cur_pde;
    PageEntries cur_target;

    bool present = false;

    int pml4;

    while(!present && pml4 < 512){
        if(*cur_pde & 1){present = true;}

        cur_target.PML4_Entry = pml4;

        cur_pde = CalculatePagePhysicalEntryAddress(&cur_target);

        pml4++;
    }
    if(pml4 >= 512){ return NULL; }
}

/*
    Used to expand the size of virtual memory, only really
    used when initializing memory because of how little virtual
    memory is created by boot2.s. Defines 1 gibibyte of virtual memory per call
*/
void* create_pd(){
    uint64_t* new_page_directory;
    uint64_t* free_pdpt;

    free_pdpt = (uint64_t*)find_free_pdpt();
    if(free_pdpt == NULL){ printf("Out of virtual memory!\n", 0); return NULL; }

    new_page_directory =  (uint64_t*)malloc(&KernelTask);

    *free_pdpt = (uint64_t)new_page_directory | 1;
    return (void*)new_page_directory;
}

/*
    Used to expand the size of virtual memory, only really
    used when initializing memory because of how little virtual
    memory is created by boot2.s. Defines 512 gibibytes of virtual memory per call.
*/
void* create_pdpt(){
    uint64_t* new_pdpt;
    uint64_t* free_pml4;

    free_pml4 = (uint64_t*)find_free_pml4();
    if(free_pml4 == NULL){ return NULL; }

    new_pdpt =  (uint64_t*)malloc(&KernelTask);

    *free_pml4 = (uint64_t)new_pdpt | 1;
    return (void*)new_pdpt;
}

void PrintMemorySize(size_t bytes){
    if(bytes == 0){ 
        printf("0 Byte(s)", 0);
    }
    
    uint64_t kib = bytes / 0x400; // kibibyte count
    uint64_t mib = bytes / 0x100000; // mibibyte count
    uint64_t gib = bytes / 0x40000000; // gibibyte count

    char print[24];

    if(gib){
        int_to_char_array(gib, print, sizeof(print), 0);
        printf(print, 0);
        printf(" GiB(s)", 0);
    }else if(mib){
        int_to_char_array(mib, print, sizeof(print), 0);
        printf(print, 0);
        printf(" MiB(s)", 0);
    }else if(kib){
        int_to_char_array(kib, print, sizeof(print), 0);
        printf(print, 0);
        printf(" KiB(s)", 0);
    }else{
        int_to_char_array(bytes, print, sizeof(print), 0);
        printf(print, 0);
        printf(" Byte(s)", 0);
    }
}

void E820_Dump_Descriptor(E820_Range_Descriptor* descriptor){
    char test_char[36];
    int_to_char_array_hex(abs(descriptor->Base_Address), test_char, sizeof(test_char), 10);
    printf(test_char, 0);
    printf("/", 0);
    int_to_char_array_hex(abs(descriptor->Length), test_char, sizeof(test_char), 10);
    printf(test_char, 0);
    printf("/ ", 0);
    PrintMemorySize(descriptor->Length);
    if(descriptor->Type == 1){
        printf(" of available memory\n", 0);
    }else{
        printf(" of unavailable memory\n", 0);
    }
}

void Enumerate_E820(){
    uintptr_t e820_ptr = (uintptr_t)E820_BUFFER_ADDRESS;
    E820_Range_Descriptor* cur_descriptor = (E820_Range_Descriptor*)e820_ptr;

    uintptr_t loop_count_ptr = (uintptr_t)(E820_BUFFER_ADDRESS-24);
    uint32_t loop_count = (*(uint32_t*)loop_count_ptr) / 24;

    char test_char[36];
    int_to_char_array(loop_count, test_char, sizeof(test_char), 0);
    printf(test_char, 0);
    printf(" memory regions detected\n", 0);

    for(int i = 0; i < loop_count-1; i++){
        E820_Dump_Descriptor(cur_descriptor);
        if(cur_descriptor->Type == 1){
            available_mem+=cur_descriptor->Length;
        }
        total_mem+=cur_descriptor->Length;

        cur_descriptor++;
    }
    SetTextColor(LCYAN, BLACK);
    printf("Available memory: ", 0);
    PrintMemorySize(available_mem);
    printf("\nUnavailable memory: ", 0);
    PrintMemorySize(total_mem-available_mem);
    printf("\nTotal Memory: ", 0);
    PrintMemorySize(total_mem);
    printf("\n", 0);
}
#include "LowLevel/Memory.h"
#include "stdbool.h"
#include "Libraries/Math.h"
#include "LowLevel/Timer.h"

// The physical address of the start of the PML4 table, loaded by init_mem from CR3
volatile uint64_t Kernel_PML4_Physical = 0;
volatile uint64_t PML4_Physical = 0;

// The following are how many of each table actually exist in memory.
// These exist in case the kernel wants to support more memory in the future
#define PML4_Count 0x01 // ALWAYS set to one
#define PDPT_Count 0x01 // 1 page directory pointer table total
#define PD_Count 0x01 // 1 page directory total
#define PT_Count 0x20 // 32 page tables total

#define KERNEL_PT_COUNT 32 // how many page tables the kernel uses

uint32_t VirtualMemorySize = 0; // the total size of the virtual memory in pages
bool mem_init = false; // used by the allocator to make sure the kernel has properly initialized virtual memory 

volatile struct Physical_Frame* phys_frames;

volatile Task* KernelTask;

// set by Enumerate_E820
volatile uint64_t available_mem;
volatile uint64_t total_mem;

uint64_t PhysicalPagesUsed = 0;

// Set a bit from the bitmap of physical space, used for fragmentation
void mem_SetBit(uint64_t address){
    phys_frames[address/0x1000].flags |= 1;
    
    PhysicalPagesUsed++;
}

// Clear a bit from the bitmap of physical space, used for fragmentation
void mem_ClearBit(uint64_t address) {
    phys_frames[address/0x1000].flags &= ~1;
    
    PhysicalPagesUsed--;
}

// Get a bit from the bitmap of physical space, used for fragmentation
uint8_t mem_GetBit(uint64_t address){
    return ((phys_frames[address/0x1000].flags) & 0b111) != 0;
}

// Sets up a new PML4 table for a user task. Follows the basic code from boot2.s. Returns the new PML4 tables physical address
uint64_t Create_User_Memory(){
    void* pml4 = malloc(KernelTask);

    uint64_t PML4_Virtual = (PML4_Physical + VIRTUAL_MEMORY_BARRIER);

    memset(pml4, 0, 0x1000);
    memcpy(pml4, (void*)PML4_Virtual, 0x1000);
    memset(pml4, 0, 0x800);

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

void mem_set_cr3(uint64_t addr, bool sti){
    // set the CR3 register to the new PML4
    asm volatile(
        "cli\n"
        "movq %0, %%cr3\n"
        : : "r" (addr) : "memory"
    );
    if(sti){
        asm volatile(
            "sti\n" 
            : : : "memory"
        );
    }
    mem_sync_cr3();
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

// Initialize virtual memory, only ever called at kernel boot
void InitMem(){
    mem_sync_cr3();

    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (Kernel_PML4_Physical)
        : : "%rax"
    );

    // zero out the identity mapped kernel, which is no longer used after kernel_setup
    memset((void*) Kernel_PML4_Physical, 0, 0x800);
    
    memset(TaskManager, 0, sizeof(TaskManager));

    TaskMemoryDefinition KernelMemory;
    KernelMemory.BaseVirtualAddress = VIRTUAL_MEMORY_BARRIER;
    KernelMemory.PageCount = 512*32;
    
    KernelTask = (Task*)&TaskManager[0];

    KernelTask->ProcessID = 0;
    KernelTask->MemoryData = KernelMemory;
    KernelTask->RequestedSleepCycle = 0;
    KernelTask->ProcessState = 0x02;
    KernelTask->MaxTicks = 5;
    KernelTask->Exists = true;
    
    VirtualMemorySize = PT_Count * PD_Count * PDPT_Count;
    mem_init = true;

    // enumerate e820 then map all of physical memory into kernel space.
    Enumerate_E820(false);

    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (Kernel_PML4_Physical)
        : : "%rax"
    );

    phys_frames = (struct Physical_Frame*)(KernelTask->MemoryData.BaseVirtualAddress + ((KernelTask->MemoryData.PageCount) * 4096));

    uint64_t safe_total_mem = (total_mem & ~(0xFFFULL)) - 0x1000;

    uint64_t temp_phys = KernelTask->MemoryData.PageCount;
    
    uint64_t phys_frames_pages = ((total_mem / 0x1000) * sizeof(struct Physical_Frame) + 0xFFF) / 0x1000;
    KernelTask->MemoryData.PageCount += phys_frames_pages;

    for(uint64_t i = 0; i < (KernelTask->MemoryData.PageCount)+1; i++){
        phys_frames[i].flags = 0b101;
    }


    // Enumerate_E820 sets 2 variables:
    // total_mem: the total amount of memory in the system.
    // available_mem: the total amount of non dirty memory in the system.
    // to calculate dirty memory, just do total_mem-available_mem.

    // since we need phys_frames to have enough memory for each physical frame, we use this math to increase page count and set those bits.

    Enumerate_E820(true);

    char percent_buffer[7];
    memset(percent_buffer, ' ', 6);
    percent_buffer[0] = '%';

    char count_buffer[23];

    uint64_t mem = Create_User_Memory();

    mem_set_cr3(mem, false);

    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (Kernel_PML4_Physical)
        : : "%rax"
    );

    KernelTask->Base_PML4 = mem;
    PML4_Physical = mem;

    PageDetails page;

    for(uint64_t phys = (512*64)*0x1000; phys < total_mem; phys+=0x1000){
        memset(&page, 0, sizeof(PageDetails));
        page.physical_address = phys;
        page.virtual_address = phys+0xffff800000000000ULL;
        page.flags.flags = KERNEL_FLAGS;
        page.flags.Execute_Disable = false;

        if((phys_frames[phys/0x1000].flags & 0b10) != 0b10){
            alloc_page(&page);
        }

        #ifdef MEM_SANITY
            //WriteString("      ", 70, 3);
            int_to_char_array((phys*100)/(total_mem-0x1000), percent_buffer+1, 7, 0);
            WriteString(percent_buffer, 70, 3);
            int_to_char_array_hex(phys, count_buffer, 23, 10);
            WriteString(count_buffer, 66, 4);
        #endif
    }

    FlushTLB();
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

bool IsCanonical(uint64_t addr) {
    // Shift right by 47 bits. 
    // If it's a "low" address, the result should be 0.
    // If it's a "high" address, the result should be 0x1FFFF (all 1s).
    // But since we want bits 48-63 to match bit 47:
    int64_t signed_addr = (int64_t)addr;
    uint64_t extension = (uint64_t)(signed_addr >> 47);
    
    // If the extension is all 0s (0) or all 1s (0xFFFFFFFFFFFFFFFF or similar), it's good.
    // Actually, a simpler way:
    return ((addr >> 47) == 0) || ((addr >> 47) == 0x1FFFFFFFFULL); 
    // Wait, the easiest way is just checking the sign extension:
    return ((int64_t)(addr << 16) >> 16) == (int64_t)addr;
}

/**
 * @brief Used by CalculatePagePhysicalEntryAddress incase a entry isn't set.
 * @param e_addr The virtual address of the entry.
 * @param flags The flags for the new entry.
 * @param flush Whether or not a full TLB flush should occur if a new entry is made.
 */
void EnsureEntry(volatile uint64_t* e_addr, uint8_t flags, bool flush){
    uint64_t fix = (*e_addr) & 0x000FFFFFFFFFF000ULL;
    if(!(*e_addr & 1ULL) || !IsCanonical(fix)){
        uint64_t new = FindNextFreePhysical();
        uint64_t virt = new+VIRTUAL_MEMORY_BARRIER;
        phys_frames[new/0x1000].flags = 0b101;

        memset((void*)virt, 0, 0x1000);

        *e_addr = (uint64_t)(
            new |
            flags |
            1ULL
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
    
    volatile uint64_t* PML4_Pointer = (uint64_t*)((PML4_Physical + (entries->PML4_Entry * 8)) + 0xffff800000000000ULL);
    
    EnsureEntry(PML4_Pointer, flags, true);

    volatile uint64_t* PDPT_Pointer = (uint64_t*)(((*PML4_Pointer & 0x000FFFFFFFFFF000ULL) + (entries->PDPT_Entry * 8)) + 0xffff800000000000ULL);

    EnsureEntry(PDPT_Pointer, flags, true);

    volatile uint64_t* PD_Pointer = (uint64_t*)(((*PDPT_Pointer & 0x000FFFFFFFFFF000ULL) + (entries->PD_Entry * 8)) + 0xffff800000000000ULL);

    EnsureEntry(PD_Pointer, flags, true);

    volatile uint64_t* PT_Pointer = (uint64_t*)(((*PD_Pointer & 0x000FFFFFFFFFF000ULL) + (entries->PT_Entry * 8)) + 0xffff800000000000ULL);

    return PT_Pointer;
}

// Allocates a single page, includes protection against memory leaks
void* alloc_page(PageDetails* page){
    uint64_t vaddr = page->virtual_address;

    PageEntries Deconstructed = ExtractPageEntries(page->virtual_address);
    uint16_t PDPT_Entry = Deconstructed.PDPT_Entry;
    uint16_t PD_Entry = Deconstructed.PD_Entry;
    uint16_t Page = Deconstructed.PT_Entry;

    PageEntries DeconstructedP = ExtractPageEntries(page->physical_address);
    uint16_t PML4_EntryP = DeconstructedP.PML4_Entry;
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    if(page->physical_address >= total_mem){
        cls();
        printf("Critical Memory Failure, attempted to map physical memory above total_mem\n");
        printf("Physical Address: %x | total_mem: %x\n", page->physical_address, total_mem);
        
        panic();
    }

    if((phys_frames[page->physical_address/0x1000].flags & 0b10) == 0b10){ 
        print("Refused to map page: DIRTY\n", 28);
        return NULL;
    }

    if((phys_frames[page->physical_address/0x1000].flags & 0b100) == 0b100 && page->virtual_address < VIRTUAL_MEMORY_BARRIER){
        printf("Refused to map page: ACCESS (p=%x, v=%x, f=%b)\n", page->physical_address, page->virtual_address, phys_frames[page->physical_address/0x1000].flags);
        return NULL;
    }

    uint64_t phys = page->physical_address;

    uint8_t page_bit = mem_GetBit(phys); // makes sure the page is actually available
    
    struct Physical_Frame* phys_frame = &phys_frames[page->physical_address/0x1000];

    if(phys_frame->refcount == 0 && page->virtual_address < VIRTUAL_MEMORY_BARRIER){
        phys_frame->flags = 0b1;
    }
    if(page->virtual_address < VIRTUAL_MEMORY_BARRIER){
        phys_frame->refcount++;
        if(phys_frame->flags & 0b100){
            printf("AHHHHH\n");
        }
    }

    // the physical address of the page entry we want to set
    uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);

    if(Page_Entry == NULL){ printf("No page entry\n"); return NULL; }

    uint64_t new_entry = 
        (page->physical_address) |
        (page->flags.flags) |
        (((uint64_t)page->flags.Execute_Disable) << 63);

    *Page_Entry = new_entry;

    // must flush the TLB with invlpg, otherwise CPU wont know the page was updated
    asm volatile("invlpg (%0)" : : "r" (page->virtual_address) : "memory");
    return (void*)(page->virtual_address);
}; 

void* alloc_page_no_invlpg(PageDetails* page){
    uint64_t vaddr = page->virtual_address;

    PageEntries Deconstructed = ExtractPageEntries(page->virtual_address);
    uint16_t PDPT_Entry = Deconstructed.PDPT_Entry;
    uint16_t PD_Entry = Deconstructed.PD_Entry;
    uint16_t Page = Deconstructed.PT_Entry;

    PageEntries DeconstructedP = ExtractPageEntries(page->physical_address);
    uint16_t PML4_EntryP = DeconstructedP.PML4_Entry;
    uint16_t PDPT_EntryP = DeconstructedP.PDPT_Entry;
    uint16_t PD_EntryP = DeconstructedP.PD_Entry;
    uint16_t PageP = DeconstructedP.PT_Entry;

    if(page->physical_address >= total_mem){
        printf("OH MY GOD STOP FUCK NO STOP\n");
        asm volatile(
            "cli\n"
            "hlt\n"
        );
    }

    if((phys_frames[page->physical_address/0x1000].flags & 0b10) == 0b10){ 
        return NULL;
    }

    uint8_t page_bit = mem_GetBit(page->physical_address); // makes sure the page is actually available
    
    struct Physical_Frame* phys_frame = &phys_frames[page->physical_address/4096];

    if(phys_frame->refcount == 0 && page->virtual_address < VIRTUAL_MEMORY_BARRIER){
        mem_SetBit(page->physical_address);
    }
    if(page->virtual_address < VIRTUAL_MEMORY_BARRIER){
        phys_frame->refcount++;
    }

     // the physical address of the page entry we want to set
    uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);

    if(Page_Entry == NULL){ printf("No page entry\n"); return NULL; }

    uint64_t new_entry = 
        (page->physical_address) |
        (page->flags.flags) |
        (((uint64_t)page->flags.Execute_Disable) << 63);

    *Page_Entry = new_entry;

    return (void*)(vaddr);
}; 

// Gets rid of a single page, includes protection against memory leaks
void* free_page(PageDetails* page){
    uint64_t vaddr = page->virtual_address;

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

    struct Physical_Frame* phys_frame = &phys_frames[page->physical_address/4096];

    uint8_t page_bit = mem_GetBit(page->physical_address); // makes sure the page is actually available
    if(phys_frame->refcount == 1){
        mem_ClearBit(page->physical_address);
    }
    phys_frame->refcount--;

    // the physical address of the page entry we want to set
    uint64_t* Page_Entry = CalculatePagePhysicalEntryAddress(&Deconstructed);
    if(!Page_Entry){ return NULL; }
        
    uint64_t new_entry = (uint64_t)0x0000;

    *Page_Entry = new_entry;

    // must flush the TLB with invlpg, otherwise CPU wont know the page was updated
    asm volatile("invlpg %0" : : "m" (*(char *)vaddr) : "memory");
    
    PageEntries physicalData = ExtractPageEntries(page->physical_address);

    return (void*)(vaddr);
}; 

uint64_t FindNextFreePhysical(){
    uint64_t phys = 0;
    for(phys = 0; phys < total_mem; phys+=0x1000){
        if(mem_GetBit(phys) == 0){
            return phys;
        }
    }
    return 0;
}

// Allocates one page based on a task
void* malloc(Task* TaskDetails){
    uint64_t VirtualBase = TaskDetails->MemoryData.BaseVirtualAddress;
    uint64_t VirtualTop = VirtualBase + (TaskDetails->MemoryData.PageCount * 0x1000);
    uint64_t NextPageAddress = VirtualTop+0x1000;

    PageDetails page;
    uint64_t free = FindNextFreePhysical();

    page.physical_address = free;
    page.virtual_address = NextPageAddress;
    page.flags.flags = KERNEL_FLAGS;
    page.flags.Execute_Disable = true;

    TaskDetails->MemoryData.PageCount += 1; 
    phys_frames[page.physical_address / 0x1000].flags = 0b101;

    void* allocated = alloc_page(&page);
    phys_frames[free / 0x1000].flags = 0b101;

    if(allocated == NULL) { 
        TaskDetails->MemoryData.PageCount -= 1; 
        phys_frames[page.physical_address / 0x1000].flags = 0;
    }

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
    volatile uint8_t* d = (volatile uint8_t*)dest;
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

    uint64_t PhysicalAddress = (*PTE & 0x000FFFFFFFFFF000ULL);
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
    
}

void memset(void* dest, uint8_t value, size_t bytes){
    volatile uint8_t* dest_m = (volatile uint8_t*) dest;
    
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
    int_to_char_array_hex(abs(descriptor->Length+descriptor->Base_Address), test_char, sizeof(test_char), 10);
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

void Enumerate_E820(bool do_dirty){
    uintptr_t e820_ptr = (uintptr_t)E820_BUFFER_ADDRESS;
    E820_Range_Descriptor* cur_descriptor = (E820_Range_Descriptor*)e820_ptr;

    uintptr_t loop_count_ptr = (uintptr_t)(E820_BUFFER_ADDRESS-24);
    uint32_t loop_count = (*(uint32_t*)loop_count_ptr) / 24;

    #ifdef MEM_SANITY
        if(do_dirty){
            char test_char[36];
            int_to_char_array(loop_count, test_char, sizeof(test_char), 0);
            print("\n", 0);
            print(test_char, 0);
            print(" memory regions detected\n", 0);
            print_debug("BASE          END          TYPE  SIZE\n",0);
        }  
    #endif

    for(int i = 0; i < loop_count-1; i++){
        #ifdef MEM_SANITY
            if(do_dirty){
                E820_Dump_Descriptor(cur_descriptor);
            }
        #endif
        total_mem+=cur_descriptor->Length;
        if(cur_descriptor->Type == 1){
            available_mem+=cur_descriptor->Length;
        }else{
            // loop through all the pages covered by the limit,
            // since they are reserved we just set them to used
            // but keep them unmapped.
            uint64_t cur_base = cur_descriptor->Base_Address;
            for(int i = 0; i < cur_descriptor->Length; i+=0x1000){
                cur_base+=0x1000;
                if(do_dirty){
                    mem_SetBit(cur_base);
                    phys_frames[cur_base/0x1000].flags &= 0b11;
                }
            }
        }

        cur_descriptor++;
    }
    #ifdef MEM_SANITY
        if(do_dirty){
            SetTextColor(LCYAN, BLACK);
            print_debug("Available memory: ", 0);
            PrintMemorySize(available_mem);
            print_debug("\nUnavailable memory: ", 0);
            PrintMemorySize(total_mem-available_mem);
            print_debug("\nTotal Memory: ", 0);
            PrintMemorySize(total_mem);
            print("\n", 0);
        }
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

    return (*info & 0x000FFFFFFFFFF000ULL) | offset;
}

uint16_t find_first_present(uint64_t* table){
    uint16_t i = 0;
    while(i <= 512 && (table[i++] & 1) != 1){}
    return i-1;
}

uint64_t FindNextFreePhysicalContinuous(size_t page_count){
    uint64_t phys = 0;
    for(phys = 0; phys < total_mem; phys+=0x1000*page_count){
        // check each page
        bool encountered_used = false;
        for(int i = 0; i < page_count; i++){
            if(mem_GetBit(phys+(0x1000*i)) == 1){ encountered_used = true; }
        }
        if(!encountered_used){ return phys; }
    }
    return 0;
}

struct Slab_Cache_Descriptor slabs[128];

void* kmalloc(size_t bytes){
    int i;
    for(i = 0; i <= 128; i++){
        if(!slabs[i].used){ break; }
    }
    if(i == 128){ return NULL; }
    if(bytes % 0x1000 > 0){ bytes &= !0xFFF; bytes+=0x1000; }
    PageDetails page;
    uint64_t phys = FindNextFreePhysicalContinuous(bytes/0x1000);
    if(phys == 0){
        printf("Couldn't find a continuous physical region of %x bytes\n", bytes);
        return NULL;
    }
    page.virtual_address = phys + VIRTUAL_MEMORY_BARRIER;
    page.physical_address = phys;
    page.flags.flags = KERNEL_FLAGS;
    page.flags.Execute_Disable = false;

    slabs[i].used = true;
    slabs[i].slab_size = bytes / 0x1000;
    slabs[i].slab = alloc_page(&page);
    for(int p = 1; p < bytes / 0x1000; bytes++){
        page.virtual_address += 0x1000;
        page.physical_address += 0x1000;
        alloc_page(&page);
    }
}

void kfree(uint64_t slab_addr){
    slab_addr &= ~0xFFF;
}
#include "kernel.h"

void SetUpShell(){
    printf("Creating Shell Task\n");

    uint64_t Shell_Entry = GemFS_Directory_to_Index(0, 1, "/bin/shell.elf");

    struct GemFS_Entry shell_entry = GemFS_ReadEntry(0, 1, Shell_Entry);

    printf("Shell_Entry: %i\n", GemFS_BlockToLBA(F0, 1, shell_entry.Start));

    LoadElf_GemFS(F0, 1, 2, Shell_Entry);
    //LoadElf(F0, 1500, 0);

    TASKMGR_set_current(0);


    for(int i = 0; i < 4; i++){
        malloc(KernelTask);
    }
    KernelTask->UserTSS.rsp0 = (uint64_t)malloc(KernelTask);
    KernelTask->Base_PML4 = Kernel_PML4_Physical;

    cls();
    
    SetTimerFrequency(1000); // the timer will go off every 1 milisecond

    pic_unmask(0x01); // enable keyboard

    pic_unmask(0x00); // enable timer
    volatile bool wait = true;
    while(wait){

    }
}

enum GemFS_DriveIDs correct_bootdrive(uint64_t boot_drive){
    if(boot_drive > 0x80 && boot_drive < 0x84){
        return H0+(boot_drive-0x80);
    }else if(boot_drive < 0x04){
        return F0+(boot_drive);
    }else{
        printf("Unable to correct Boot Drive into a GemFS ID.\n");
        asm volatile(
            "int $0xFE\n"
        );
        asm volatile(
            "cli\n"
            "hlt\n"
        );
    }
}

void kernel_startup(uint64_t boot_drive){
    cls();

    print("Initializing Modular GDT ", 0);
    take_print_snapshot();

    // set up the GDT, it's called "modular" because kernel code can safely change it willingly
    SetGDTEntry(0, 0, 0, 0, 0); // null entry
    SetGDTEntry(0, 0, GDT_Flags_Code, GDT_Access_Kernel_Code, 0x8); // kernel code
    SetGDTEntry(0, 0, GDT_Flags_Data, GDT_Access_Kernel_Data, 0x10); // kernel data
    SetGDTEntry(0, 0, GDT_Flags_Code, GDT_Access_User_Code, 0x18); // user code
    SetGDTEntry(0, 0, GDT_Flags_Data, GDT_Access_User_Data, 0x20); // user data

    // establish the TSS
    SetActiveTSS(0xffff800000FFFF00, 0, 0, 0xffff80000003FFF0, 0xffff80000003EFF0, 0xffff80000003DFF0, 0xffff80000003CFF0, 0xffff80000003BFF0, 0xffff80000003AFF0, 0xffff800000039FF0, 0x10000);
    SetGDTSystemEntry((uint64_t)&ActiveTSS, sizeof(ActiveTSS), GDT_Flags_Data, GDT_Access_TSS, 0x28);

    LoadGDT();

    print_success_snapshot("SUCCESS\n", 0);

    boot_drive &= 0xFF;

    print("Setting up paging ", 0);
    take_print_snapshot();

    InitMem();
    
    memcpy(&KernelTask->UserTSS, &ActiveTSS, sizeof(struct TSS));

    print_success_snapshot("SUCCESS\n", 0);
    
    print("Loading IDT ", 0);
    take_print_snapshot();
    pic_remap(0x20, 0x28);
    for(int i = 0; i < 256; i++){
        SetIDTEntry(i, (uint64_t)kernel_panic_stub, 0x08, 0x8E, 0x04);
    }
    #ifdef DEBUG
        print_debug("\nINDEX SELECTOR FLAGS IST\n", 0);
    #endif
    SetIDTEntry(0x06, (uint64_t)invalid_opcode_stub, 0x08, 0x8E, 0x04);
    SetIDTEntry(0x0D, (uint64_t)gpf_stub, 0x08, 0x8E, 0x04);
    SetIDTEntry(0x0E, (uint64_t)page_fault_stub, 0x08, 0x8E, 0x00);
    SetIDTEntry(0x80, (uint64_t)isr80_stub, 0x08, 0xEF, 0x00);
    SetIDTEntry(0x20, (uint64_t)timer_interrupt_stub, 0x08, 0x8E, 0x00);
    SetIDTEntry(0x28, (uint64_t)sync_time_stub, 0x08, 0x8E, 0x01);
    LoadIDT();
    outb(0x70, 0x8B);
    char previous = inb(0x71);
    outb(0x70, 0x8B);
    outb(0x71, previous | 0x40);
    // entry number, stub, selector, flags, ist
    SetIDTEntry(0x21, (uint64_t)keyboard_stub, 0x08, 0x8E, 0x00);
    print_success_snapshot("SUCCESS\n", 0);
    
    print("Enabling Interrupts ", 0);
    take_print_snapshot();
    asm volatile("sti" ::: "memory");
    printf("CR3 %x ", PML4_Physical);
    print_success_snapshot("SUCCESS\n", 0);

    enum GemFS_DriveIDs boot_driveid = correct_bootdrive(boot_drive);

    print("Scanning Busses ", 0);
    take_print_snapshot();
    ScanBusses();
    print_success_snapshot("SUCCESS\n", 0);

    if(AHCI_Controller == NULL){ 
        print_error("No AHCI drive detected\n", 0); 
    }else{
        print("Initializing AHCI Drive ", 0);
        take_print_snapshot();
        AHCI_Init(AHCI_Controller);
    }

    FLOPPY_Check_FDC();

    if(FLOPPY_FDC_Present == NULL){ 
        print_error("No Floppy drive detected\n", 0); 
    }else{
        print("Initializing Floppy Drive Controller ", 0);
        take_print_snapshot();
        int floppy_status = FLOPPY_Init_Controller();
        
        print("Initializing Floppy Drive 0 ", 0);
        take_print_snapshot();
        FLOPPY_Init_Drive(0);
    }

    FLOPPY_Configure(0, true, false, true);
    
    printf("DOING FLOPPY READ TEST ");
    take_print_snapshot();
    printf("\n");
    // at boot0.s line 279-280 i put a byte that reads 69 (which would be placed at byte 439 of the first sector)
    uint8_t test_floppy_buffer[512];
    FLOPPY_Read_CHS(0, 0, 0, 1, (uint64_t)test_floppy_buffer, 1);
    #ifdef DEBUG
        printf("READ %i FROM BYTE 439 OF 0:0:1\n", test_floppy_buffer[439]);
        printf("EXPECTED 69\n");
    #endif
    if(test_floppy_buffer[439] == 69){
        #ifdef DEBUG
            print_success_snapshot("SUCCESS", 0);
        #else
            print_success_snapshot("SUCCESS\n", 0);
        #endif
    }else{
        #ifdef DEBUG
            print_error_snapshot("FAILURE", 0);
        #else
            print_error_snapshot("FAILURE\n", 0);
        #endif
    }
    
    printf("Initialzing GemFS ");
    take_print_snapshot();
    printf("\n");

    GemFS_Init(boot_driveid);

    print_success_snapshot("SUCCESS", 8);

    char class = 'u';

    #if BUILD_CLASS == 0x02
        class = 'r';
    #endif

    printf("ChudOS Version %i.%i.%i:%i%c\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, BUILD, class);

    printf("Mem data: %x\n", available_mem); 

    #ifdef TEST_USER
        SetUpShell();
    #endif

    barrier();

    while (1){

    }
}

void kernel_main(uint64_t BootDrive){ 
    kernel_startup(BootDrive);
    return;
}
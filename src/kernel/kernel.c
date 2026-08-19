#include "kernel.h"

void SetUpShell(){
    virtualprint(KERNEL_T, "[boot.shell] init\n");

    uint64_t Shell_Entry = GemFS_Directory_to_Index(0, 1, "/bin/shell.elf");

    struct GemFS_Entry shell_entry = GemFS_ReadEntry(0, 1, Shell_Entry);

    LoadElf_GemFS(F0, 1, 2, Shell_Entry);
    //LoadElf(F0, 1500, 0);

    TASKMGR_set_current(0);

    for(int i = 0; i < 4; i++){
        malloc(KernelTask);
    }
    KernelTask->UserTSS.rsp0 = (uint64_t)malloc(KernelTask);
    KernelTask->Base_PML4 = Kernel_PML4_Physical;
    
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

    initVirtualTerminals(true);

    virtualprint(KERNEL_T, "[boot] start\n");

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

    virtualprint(KERNEL_T, "[boot.gdt] success\n");

    boot_drive &= 0xFF;

    InitMem();
    
    memcpy(&KernelTask->UserTSS, &ActiveTSS, sizeof(struct TSS));

    virtualprint(KERNEL_T, "[boot.mem] success\n");

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
    virtualprint(KERNEL_T, "[boot.idt] success\n");

    asm volatile("sti" ::: "memory");
    virtualprint(KERNEL_T, "[boot.int] success\n");

    enum GemFS_DriveIDs boot_driveid = correct_bootdrive(boot_drive);

    ScanBusses();
    virtualprint(KERNEL_T, "[boot.pci] success\n");

    virtualprint(KERNEL_T, "[boot.ahci] init\n");
    if(AHCI_Controller == NULL){ 
        virtualprint(KERNEL_T, "[boot.ahci] no drive\n"); 
    }else{
        AHCI_Init(AHCI_Controller);
        virtualprint(KERNEL_T, "[boot.ahci] success\n");
    }

    FLOPPY_Check_FDC();

    if(FLOPPY_FDC_Present == NULL){ 
        virtualprint(KERNEL_T, "[boot.fdc] no fdc\n"); 
    }else{
        
        int floppy_status = FLOPPY_Init_Controller();
        
        virtualprint(KERNEL_T, "[boot.fdc] init 0\n");
        
        FLOPPY_Init_Drive(0);
    }
    virtualprint(KERNEL_T, "[boot.fdc] success\n");

    FLOPPY_Configure(0, true, false, true);
    
    virtualprint(KERNEL_T, "[boot.floppy] test\n");

    // at boot0.s line 279-280 i put a byte that reads 69 (which would be placed at byte 439 of the first sector)
    uint8_t test_floppy_buffer[512];
    FLOPPY_Read_CHS(0, 0, 0, 1, (uint64_t)test_floppy_buffer, 1);
    if(test_floppy_buffer[439] == 69){
        virtualprint(KERNEL_T, "[boot.floppy] success\n");
    }else{
        virtualprint(KERNEL_T, "[boot.floppy] failure\n");
    }
    
    virtualprint(KERNEL_T, "[boot.gemfs] init\n");

    GemFS_Init(boot_driveid);

    virtualprint(KERNEL_T, "[boot.gemfs] success\n");

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
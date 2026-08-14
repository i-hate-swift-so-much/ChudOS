#include "filesys/gemfs.h"
#include "kernel.h"

struct GemFS_DriveData Drives[16];

uint8_t Entry_Buffer[512];
uint64_t GemFS_FBB_Size;

void GemFS_FormatPartition(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Formatting partition %i of drive %i\n", Partition, DriveID);
        SetTextColor(WHITE, BLACK);
    #endif
    
    struct GemFS_DriveData Drive = Drives[DriveID];
    struct GemFS_MBRPartition Part = Drive.PartitionTable[Partition];

    struct GemFS_Main Main_Entry;
    memset(&Main_Entry, 0, sizeof(Main_Entry));

    char magic[4] = {'G','E','M','H'};

    memcpy(&Main_Entry.check, &magic, 4);
    Main_Entry.Block_Size = 1;
    Main_Entry.Entry.Flags = 0b00010111;

    Main_Entry.Entry.Start = 0;
    Main_Entry.Entry.Next_Index_Start = 0;
    Main_Entry.Entry.Parent_Index = 0;
    Main_Entry.Entry.Sibling_Index = 0;

    char* name = "ROOT";

    Main_Entry.Entry.Index = 0;
    memcpy(&Main_Entry.Entry.Name, name, 5);

    memset(Entry_Buffer, 0, 512);

    Drives[DriveID].Main_Entries[Partition] = Main_Entry;

    memcpy(Entry_Buffer, &Main_Entry, sizeof(Main_Entry));
    FLOPPY_Write_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, 1), (uint64_t)Entry_Buffer, 1);  

    // set up FBB
    GemFS_FBB_Size = 8;
    memset(Entry_Buffer, 0, 512);
    FLOPPY_Write_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, 2), (uint64_t)Entry_Buffer, 1);
    for(int i = 0; i < 6; i++){
        GemFS_FBB_SetBlock(DriveID, Partition, i);
    }

    // create the basic directories
    /*
        /
        |--- dev/
        |    |
        |    |--- zero/
        |    +--- urand/
        |--- etc/
        |    |
        |    +--- boot_msg.txt
        |--- bin/
        |    |
        |    +--- shell.elf
        |    +--- hello_world.elf
        |    +--- chudedit.elf
    */
    uint64_t dev_entry = GemFS_mkdir(DriveID, Partition, "dev", 4, 0b00010111, 0);
    GemFS_mkdir(DriveID, Partition, "zero", 5, 0b00010111, 1);
    GemFS_mkdir(DriveID, Partition, "urand", 6, 0b00010111, 1);
    uint64_t etc_entry = GemFS_mkdir(DriveID, Partition, "etc", 4, 0b00010111, 0);
    uint64_t bin_entry = GemFS_mkdir(DriveID, Partition, "bin", 4, 0b00010111, 0);

    uint64_t shell_entry = GemFS_CreateFile(DriveID, Partition, "shell.elf", 9, 0b00001111, bin_entry, 9);
    struct GemFS_Entry entry = GemFS_ReadEntry(DriveID, Partition, shell_entry);
    entry.Start = GemFS_LBAToBlock(DriveID, Partition, 1500);
    GemFS_WriteEntry(F0, Partition, shell_entry, entry);

    uint64_t hello_entry = GemFS_CreateFile(DriveID, Partition, "hello_world.elf", 15, 0b00001111, bin_entry, 9);
    struct GemFS_Entry hentry = GemFS_ReadEntry(DriveID, Partition, hello_entry);
    hentry.Start = GemFS_LBAToBlock(DriveID, Partition, 1600);
    GemFS_WriteEntry(F0, Partition, hello_entry, hentry);

    uint64_t chudedit_entry = GemFS_CreateFile(DriveID, Partition, "chudedit.elf", 12, 0b00001111, bin_entry, 9);
    struct GemFS_Entry ceentry = GemFS_ReadEntry(DriveID, Partition, chudedit_entry);
    ceentry.Start = GemFS_LBAToBlock(DriveID, Partition, 1800);
    GemFS_WriteEntry(F0, Partition, chudedit_entry, ceentry);

    int i = 0;
    int last_i = 0;

    uint8_t boot_msg_buffer[0x200];
    memset(boot_msg_buffer, 0, 0x200);
    char* msg_start = "ChudOS Version ";
    for(i = 0; i < 0x200; i++){
        if(msg_start[i] == '\0'){ last_i = i; break; }
        boot_msg_buffer[i] = msg_start[i];
    }
    char msg_num_char[26];
    int_to_char_array(VERSION_MAJOR, msg_num_char, 26, 0);
    for(i = 0; i < 0x200; i++){
        if(msg_num_char[i] == '\0'){ last_i += i; break; }
        boot_msg_buffer[last_i] = msg_num_char[i];
    }
    boot_msg_buffer[last_i] = '.';
    last_i++;
    int_to_char_array(VERSION_MINOR, msg_num_char, 26, 0);
    for(i = 0; i < 0x200; i++){
        if(msg_num_char[i] == '\0'){ last_i += i; break; }
        boot_msg_buffer[last_i+i] = msg_num_char[i];
    }
    boot_msg_buffer[last_i] = '.';
    last_i++;
    int_to_char_array(VERSION_PATCH, msg_num_char, 26, 0);
    for(i = 0; i < 0x200; i++){
        if(msg_num_char[i] == '\0'){ last_i += i; break; }
        boot_msg_buffer[last_i+i] = msg_num_char[i];
    }
    boot_msg_buffer[last_i] = ':';
    last_i++;
    memset(msg_num_char, 0, 26);
    int_to_char_array(BUILD, msg_num_char, 26, 0);
    for(i = 0; i < 0x200; i++){
        if(msg_num_char[i] == '\0'){ last_i += i; break; }
        boot_msg_buffer[last_i+i] = msg_num_char[i];
    }
    boot_msg_buffer[last_i] = 'u';
    #if BUILD_CLASS == 0x02
        boot_msg_buffer[last_i] = 'r';
    #endif
    boot_msg_buffer[last_i+1] = '\0';
    
    uint64_t boot_msg_entry = GemFS_CreateFile(DriveID, Partition, "boot_msg.txt", 13, 0b00001111, etc_entry, 1);
    GemFS_WriteFile(boot_msg_buffer, 0x200, DriveID, Partition, boot_msg_entry);
}

uint64_t GemFS_BlockToLBA(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Block){
    uint16_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;
    if(BS == 0){ BS = 1; }
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partition];
    return (Block * BS) + part.LBA_Start;
}

uint64_t GemFS_LBAToBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t LBA){
    uint16_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;
    if(BS == 0){ BS = 1; }
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partition];
    return (LBA - part.LBA_Start) / BS;
}

uint64_t FBB_Read[64];

void GemFS_GetFBB(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partition];
    FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, 2), (uint64_t)FBB_Read, 1);
    GemFS_FBB_Size = FBB_Read[0];
}

uint8_t FBB_Block[4096];

/**
 * @brief Finds a free block in the FBB for use
 */
uint64_t GemFS_FindFreeBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partition];
    uint16_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;
    uint64_t size = (part.Sector_Count) / (uint64_t)BS;
    
    for(uint64_t i = 0; i < 0x200; i++){
        if(!GemFS_FBB_GetBlock(DriveID, Partition, i)){
            return i;
        }
    }
    return size+1;
}

uint64_t GemFS_mkdir(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* name, size_t name_len, uint8_t flags, uint64_t ParentIndex){
    uint64_t free = GemFS_FindFreeEntry(F0, 1);

    uint64_t free_block = GemFS_FindFreeBlock(DriveID, Partition);
    GemFS_FBB_SetBlock(F0, Partition, free_block);

    struct GemFS_Entry new_dir = GemFS_ReadEntry(DriveID, Partition, free);

    memset(&new_dir, 0, sizeof(struct GemFS_Entry));
    new_dir.Flags = flags;
    new_dir.Index = free;
    new_dir.Parent_Index = ParentIndex;
    new_dir.ReadingMask = 0;
    new_dir.Size = 0;
    new_dir.Start = 0;
    new_dir.Sibling_Index = 0;

    struct GemFS_Entry parent_entry = GemFS_ReadEntry(DriveID, Partition, ParentIndex);
    if(parent_entry.Flags & 0b00010000 != 0b00010000){
        printf("<GemFS> Cannot give files a child.\n");
    }
    if(parent_entry.Start != 0){
        // if needed, scan through the siblings and regiter it as a sibling
        struct GemFS_Entry cur_entry = GemFS_ReadEntry(DriveID, Partition, parent_entry.Start);
        int i = cur_entry.Index;
        while(cur_entry.Sibling_Index != 0){
            cur_entry = GemFS_ReadEntry(DriveID, Partition, cur_entry.Sibling_Index);
            i++;
        }
        cur_entry.Sibling_Index = free;
        GemFS_WriteEntry(DriveID, Partition, cur_entry.Index, cur_entry);
    }

    memcpy(&new_dir.Name, name, name_len);

    GemFS_WriteEntry(F0, Partition, free, new_dir);
    
    if(parent_entry.Start == 0){
        parent_entry.Start = free;
    }

    GemFS_WriteEntry(DriveID, Partition, ParentIndex, parent_entry);

    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("<GemFS> Created directory ");
        printf(name);
        printf(" with index %i and parent index %i (", free, ParentIndex);
        printf(parent_entry.Name);
        printf(")\n");
        SetTextColor(WHITE, BLACK);
    #endif

    return free;
}

uint64_t GemFS_CreateFile(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* name, size_t name_len, uint8_t flags, uint64_t ParentIndex, uint64_t Size){
    uint64_t free_block = GemFS_FindFreeBlock(DriveID, Partition);
    
    for(int i = 0; i < Size; i++){
        GemFS_FBB_SetBlock(DriveID, Partition, i+free_block);
    }
    
    uint64_t free = GemFS_FindFreeEntry(DriveID, Partition);
    
    GemFS_mkdir(DriveID, Partition, name, name_len, flags, ParentIndex);

    struct GemFS_Entry entry;
    memset(&entry, 0, sizeof(struct GemFS_Entry));
    entry = GemFS_ReadEntry(DriveID, Partition, free);
    entry.Start = free_block;
    entry.Size = Size;

    uint64_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;

    GemFS_WriteEntry(DriveID, Partition, free, entry);

    return free;
}

void GemFS_WriteFile(void* buffer, size_t buffer_size, enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index){
    struct GemFS_Entry entry;
    memset(&entry, 0, sizeof(struct GemFS_Entry));
    entry = GemFS_ReadEntry(DriveID, Partition, Index);

    if((entry.Flags >> 4) & 1 == 1){
        #ifdef GEMFS_SANITY
            SetTextColor(LRED, BLACK);
            printf("<GemFS> Tried to write to directory ");
            printf(entry.Name);
            printf(" as if it is a file\n");
            SetTextColor(WHITE, BLACK);
        #endif
        return;
    }

    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("<GemFS> Wrote %x blocks to file index %i\n", entry.Size, Index);
        SetTextColor(WHITE, BLACK);
    #endif

    uint64_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;

    FLOPPY_Write_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, entry.Start), (uint64_t)buffer, entry.Size*BS);

    GemFS_WriteEntry(DriveID, Partition, Index, entry);
}

void GemFS_ReadFile(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t index, void* buffer, uint64_t Blocks){
    struct GemFS_Entry entry = GemFS_ReadEntry(DriveID, Partition, index);

    if((entry.Flags >> 4) & 1){
        printf("<GemFS> Tried to read file that was labeled as a directory ");
        printf(entry.Name);
        printf(" Flags: %b. Index: %i\n", entry.Flags, index);
        return;
    }

    FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, entry.Start), (uint64_t)buffer, Blocks);
}

bool GemFS_Names_Equal(char* name1, char* name2, size_t len1, size_t len2){
    if(len1 != len2){ return false; }
    for(int i = 0; i < len1; i++){
        if(name1[i] == 0x0 && name2[i] == 0x0) { 
            return true;
        }else if(name1[i] != name2[i]){
            return false;
        }
    }
    return true;
}

uint64_t GemFS_Find_Index_By_Name(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* name, uint64_t Parent_Index){
    struct GemFS_Entry Parent_Entry;
    memset(&Parent_Entry, 0, sizeof(struct GemFS_Entry));
    Parent_Entry = GemFS_ReadEntry(DriveID, Partition, Parent_Index);

    if(Parent_Entry.Start == Parent_Entry.Index){
        #ifdef GEMFS_SANITY
            SetTextColor(LRED, BLACK);
            printf("<GemFS> Was unable to fetch ");
            printf(name);
            printf(" Given child index (%i) is equal to Parent's index\n", Parent_Entry.Start, Parent_Entry.Index);
            SetTextColor(WHITE, BLACK);
        #endif
        return 0;
    }

    struct GemFS_Entry Cur_Child;
    memset(&Cur_Child, 0, sizeof(struct GemFS_Entry));
    Cur_Child = GemFS_ReadEntry(DriveID, Partition, Parent_Entry.Start);

    uint64_t last_index = Cur_Child.Index;

    while(!GemFS_Names_Equal(name, Cur_Child.Name, calculate_string_length(name), calculate_string_length(Cur_Child.Name)) && Cur_Child.Sibling_Index != 0){
        Cur_Child = GemFS_ReadEntry(DriveID, Partition, Cur_Child.Sibling_Index);
        last_index = Cur_Child.Index;
    }

    if(Cur_Child.Sibling_Index == 0 && !GemFS_Names_Equal(name, Cur_Child.Name, calculate_string_length(name), calculate_string_length(Cur_Child.Name))){
        #ifdef GEMFS_SANITY
            SetTextColor(LRED, BLACK);
            printf("<GemFS> ");
            printf(Parent_Entry.Name);
            printf(" has no children named ");
            printf(name);
            printf("\n");
        #endif
        return -1;
    }

    last_index = Cur_Child.Index;
    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("<GemFS> Found entry named ");
        printf(name);
        printf(" at index %i\n", last_index);
        SetTextColor(WHITE, BLACK);
    #endif

    return last_index;
}

uint64_t GemFS_Directory_to_Index(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* directory){
    int i = 0;
    int cur_index = 0;
    char cur = directory[i];

    int last_i = i;
    char scan[128]; // contains the scanned letters

    bool should_scan = true;

    int len = calculate_string_length(directory);

    if (directory[0] == '/'){
        i++;
        last_i = i;
    }

    while(should_scan){
        cur = directory[i];
        if(cur == '/' || cur == '\0'){
            i++;
            last_i = i;
            cur_index = GemFS_Find_Index_By_Name(DriveID, Partition, scan, cur_index);
            if(cur_index == -1){ return -1; }
            memset(scan, 0, 128);
            if(cur == '\0'){
                should_scan = false;
                break;
            }
        }
        cur = directory[i];
        scan[i-last_i] = cur;
        i++;
    }

    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("<GemFS> Converted directory ");
        printf(directory);
        printf(" to index %i\n", cur_index);
        SetTextColor(WHITE, BLACK);
    #endif

    return cur_index;
}

/**
 * @brief Checks whether or not a block is in use through the FBB
 */
bool GemFS_FBB_GetBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Block){
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partition];
    uint16_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;
    uint64_t t_block = Block / (BS*512);
    uint64_t t_offset = Block / BS;
    uint64_t t_bit = Block % 8;

    FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, t_block+4), (uint64_t)FBB_Block, BS);

    return ((FBB_Block[t_offset] >> t_bit) & 1) == 1;
}

/**
 * @brief Sets a block to be in use through the FBB
 */
void GemFS_FBB_SetBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Block){
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partition];
    uint16_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;
    uint64_t t_block = Block / (BS*512);
    uint64_t t_offset = Block / BS;
    uint64_t t_bit = Block % 8;

    FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, t_block+4), (uint64_t)FBB_Block, BS);

    FBB_Block[t_offset] = FBB_Block[t_offset] | (1 << t_bit);

    FLOPPY_Write_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, t_block+4), (uint64_t)FBB_Block, BS);
}

uint8_t Enumeration_Buffer[512];

uint64_t GemFS_GetEntryBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index){
    struct GemFS_Main* main_entry = &Drives[DriveID].Main_Entries[Partition];
    
    struct GemFS_Entry entry = main_entry->Entry;

    uint64_t last_entry = entry.Next_Index_Start;

    if(Index == 0){ return 1; }

    for(uint64_t i = 1; i < Index+1; i++){
        if(entry.Next_Index_Start == 0){
            GemFS_CreateEntry(DriveID, Partition, i, 0);
            FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, last_entry), (uint64_t)Enumeration_Buffer, 1);
            memcpy(&entry, Enumeration_Buffer, sizeof(struct GemFS_Entry));
        }
        last_entry = entry.Next_Index_Start;
        FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, entry.Next_Index_Start), (uint64_t)Enumeration_Buffer, 1);
        memcpy(&entry, Enumeration_Buffer, sizeof(struct GemFS_Entry));
    }
    return last_entry;
}

struct GemFS_Entry GemFS_ReadEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index){
    uint64_t Block = GemFS_GetEntryBlock(DriveID, Partition, Index);
    
    if(Index == 0){
        #ifdef GEMFS_SANITY
            SetTextColor(LCYAN, BLACK);
            printf("<GemFS> Reading entry %i from block: %x\n", Index, Block);
            SetTextColor(WHITE, BLACK);
        #endif
        return GemFS_ReadMainEntry(DriveID, Partition).Entry;
    }
    
    struct GemFS_Entry ret;
    memset(&ret, 0, sizeof(ret));
    memset(&Entry_Buffer, 0, 512);

    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("<GemFS> Reading entry %i from block %x\n", Index, Block);
        SetTextColor(WHITE, BLACK);
    #endif

    FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, Block), (uint64_t)Entry_Buffer, 1);
    memcpy(&ret, &Entry_Buffer, sizeof(ret));
    return ret;
}

void GemFS_WriteEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index, struct GemFS_Entry entry){
    uint64_t Block = GemFS_GetEntryBlock(DriveID, Partition, Index);
    if(Index == 0){
        struct GemFS_Main main = Drives[DriveID].Main_Entries[Partition];
        main.Entry = entry;
        memcpy(Entry_Buffer, &main, sizeof(struct GemFS_Main));
        FLOPPY_Write_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, 1), (uint64_t)Entry_Buffer, 1);
        #ifdef GEMFS_SANITY
            SetTextColor(LCYAN, BLACK);
            printf("<GemFS> Writing entry %i to block: %x (Main)\n", Index, Block);
        #endif
        Drives[DriveID].Main_Entries[Partition] = main;
        return;
    }

    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("<GemFS> Writing entry %i to block: %x\n", Index, Block);
        SetTextColor(WHITE, BLACK);
    #endif
    memset(&Entry_Buffer, 0, 512);
    memcpy(&Entry_Buffer, &entry, sizeof(entry));
    FLOPPY_Write_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, Block), (uint64_t)Entry_Buffer, 1);
}

uint64_t GemFS_FindFreeEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_Entry entry = Drives[DriveID].Main_Entries[Partition].Entry;
    uint64_t i = 1;
    while(entry.Flags & 1){
        entry = GemFS_ReadEntry(DriveID, Partition, i);
        i++;
    }
    return i - 1;
}

struct GemFS_Main GemFS_ReadMainEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partition];

    struct GemFS_Main ret;
    memset(&ret, 0, sizeof(ret));

    
    if(DriveID < H0){
        FLOPPY_Read_LBA(DriveID, GemFS_BlockToLBA(DriveID, Partition, 1), (uint64_t)Entry_Buffer, 1);
        memcpy(&ret, &Entry_Buffer, sizeof(ret));
    }
    Drives[DriveID].Main_Entries[Partition] = ret;
    return ret;
}

void GemFS_DumpPartition(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_MBRPartition Table = Drives[DriveID].PartitionTable[Partition];
    SetTextColor(LCYAN, BLACK);
    printf("DRIVE PARTITION DUMP ID: %x\n\t", DriveID);
    printf("STATUS: %x\n\t", Table.Status);
    printf("TYPE: %x\n\t", Table.Type);
    printf("LBA_START: %x\n\t", Table.LBA_Start);
    printf("SECTOR_COUNT: %x\n", Table.Sector_Count);
    SetTextColor(WHITE, BLACK);
}

void GemFS_DumpDriveData(enum GemFS_DriveIDs DriveID){
    struct GemFS_DriveData Drive = Drives[DriveID];
    SetTextColor(LCYAN, BLACK);
    printf("DRIVE DATA DUMP ID: %x\n\t", Drive.DriveID);
    printf("IS_GEMFS: %i\n\t", Drive.Is_GemFS);
    printf("PARTITION TABLE:\n\t\t");
}

void GemFS_DumpEntryData(struct GemFS_Entry entry){
    SetTextColor(LCYAN, BLACK);
    printf("ENTRY DATA DUMP\n\t");
    printf("INDEX: %i\n\t", entry.Index);
    printf("FLAGS: %b\n\t", entry.Flags);
    SetTextColor(WHITE, BLACK);
}

void GemFS_Init(enum GemFS_DriveIDs DriveID){
    GemFS_LoadPartitionTable(DriveID);
    Drives[DriveID].DriveID = DriveID;

    char magic[4] = {'G','E','M','H'};

    struct GemFS_DriveData Drive = Drives[DriveID];

    // scan through the table to find out which partition tables are formatted as 0x7E
    for(int i = 0; i < 4; i++){
        struct GemFS_MBRPartition partition = Drive.PartitionTable[i];

        // check for the format code and if it has a main entry
        if(partition.Type == 0xC8){
            struct GemFS_Main main_entry = GemFS_ReadMainEntry(DriveID, i);

            bool correct = (
                (main_entry.check[0] == magic[0]) && 
                (main_entry.check[1] == magic[1]) &&
                (main_entry.check[2] == magic[2]) &&
                (main_entry.check[3] == magic[3])
            );

            if(correct){
                printf("ITS FORMATTED\n");
                GemFS_DumpEntryData(main_entry.Entry);
            }else{
                #ifdef GEMFS_SANITY
                    SetTextColor(YELLOW, BLACK);
                    printf("<GemFS> Drive %i Partition %i is labeled as GemFS, but not formatted.\n", DriveID, i);
                    SetTextColor(WHITE, BLACK);
                #endif
                GemFS_FormatPartition(DriveID, i);
            }
        }
    }
}

uint8_t SectorBuffer[512];

void GemFS_LoadPartitionTable(enum GemFS_DriveIDs DriveID){
    if(DriveID < H0){
        // read the MBR from the floppy
        FLOPPY_Read_LBA(DriveID, 0, (uint64_t)SectorBuffer, 1);
        memcpy(&Drives[DriveID].PartitionTable, &SectorBuffer[446], sizeof(Drives[DriveID].PartitionTable)); // copy the partition table
    }else{

    }
}

void GemFS_CreateEntry(enum GemFS_DriveIDs DriveID, uint8_t Partititon, uint64_t Index, uint8_t flags){
    #ifdef GEMFS_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("<GemFS> Creating entry %i at block ", Index);
    #endif
    
    struct GemFS_MBRPartition part = Drives[DriveID].PartitionTable[Partititon];
    uint64_t free_block = GemFS_FindFreeBlock(DriveID, Partititon);

    #ifdef GEMFS_SANITY
        printf("%x, LastEntry=%i\n", free_block, Index-1);
        SetTextColor(WHITE, BLACK);
    #endif

    GemFS_FBB_SetBlock(DriveID, Partititon, free_block);

    struct GemFS_Entry Entry;
    Entry.Index = Index;
    Entry.Flags = flags;
    Entry.Next_Index_Start = 0;

    struct GemFS_Entry LastEntry = GemFS_ReadEntry(DriveID, Partititon, Index-1);
    LastEntry.Next_Index_Start = free_block;
    GemFS_WriteEntry(DriveID, Partititon, Index-1, LastEntry);
    GemFS_WriteEntry(DriveID, Partititon, Index, Entry);
}
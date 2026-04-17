#include "Devices/PS2/Keyboard.h"

#define KBD_PORT 0x60

bool capslock = false;
bool upper = false;
bool ctrl = false;
bool alt = false;

bool should_proceed = false;
bool should_not_proceed = false;

uint8_t lastScancode = 0x00;

char KeyCodeLookupTable[] ={ 
    0, // null
    27, // escape
    '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']',
    0,0,'a','s','d','f','g','h','j','k','l',';',0x27,'`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',
    0,' ',0,0,0,0,0,0,0,0,0,0,0,0,
    '7','8','9','-',
    '4','5','6','+',
    '1','2','3','0','.',0,0
};

char KeyCodeLookupTable_Shift[] = {
    0, // null
    27, // escape
    '!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}',
    0,0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',
    0,' ',0,0,0,0,0,0,0,0,0,0,0,0,
    '7','8','9','-',
    '4','5','6','+',
    '1','2','3','0','.',0,0
};

char KeyCode_To_Char(enum PS2_KeyCodesPressed Keycode){
    if(upper ^ capslock){
        return KeyCodeLookupTable_Shift[Keycode];
    }else{
        return KeyCodeLookupTable[Keycode];
    }
}

uintptr_t canonicalize_arithmetic(uintptr_t addr) {
    // This expression clears the top 16 bits, then arithmetically
    // extends bit 47 to bits 48-63 using a specific bitwise trick
    return (addr & ((1ULL << 48) - 1)) | ~(((addr & (1ULL << 47)) - 1));
}

bool KeycodeIsPrintable(enum PS2_KeyCodesPressed Keycode){
    return (
        KeyCodeLookupTable[Keycode] != 0
    );
}

int GetWaitingProgram(){
    for(int i = 0; i < 512; i++){
        if(TaskManager[i].Exists){
            if(TaskManager[i].ProcessState == WAITING_PROCESS_STATE && TaskManager[i].WaitingReason == WAITING_REASON_INPUT){
                return i;
            }
        }
    }
    return 512;
}

char key_buffer[256];
int key_buffer_pos = 0;

void HandleKeyboardInterrupt(InterruptRegisters* regs){
    int cur = TASKMGR_get_current();
    volatile Task* cur_task = (volatile Task*)&TaskManager[cur];
    task_switch_frame(&cur_task->SavedRegisters, regs);
    
    bool enter = false;

    if(inb(0x64) & 1){
        enum PS2_KeyCodesPressed Keycode = (enum PS2_KeyCodesPressed)inb(KBD_PORT);

        bool release = false;

        if(Keycode >= 0x81 && Keycode <= 0xD8){
            release = true;
            Keycode -= 0x80;
        }

        if(Keycode == L_SHIFT || Keycode == R_SHIFT){
            upper = !release;
        } else if(Keycode == CAPSLOCK){
            capslock = !capslock;
        }
        else if(Keycode > NULL_CODE && Keycode <= FU12 && !release){
            int pid = GetWaitingProgram();
            if(KeycodeIsPrintable(Keycode)){
                char to_print = KeyCode_To_Char(Keycode);
                if(to_print == '\b' && key_buffer_pos > 0){ key_buffer_pos--; print("\b", 1); } else
                if(to_print == '\t' && key_buffer_pos < 252){ memset(&key_buffer[key_buffer_pos+1], ' ', 4); key_buffer_pos+=4; print("\t", 1); } 
                else if(key_buffer_pos < 256){
                    key_buffer[key_buffer_pos] = to_print;
                    key_buffer_pos++;
                    print(&to_print, 1);
                }
            }else if(Keycode == ENTER){
                if(pid == 512){
                    pic_send_eoi(0x01);
                    return;
                }
                //printf("ENTER FOR %i\n", pid);
                volatile Task* waiting_task = (volatile Task*)&TaskManager[pid];

                // since it's enter, and we're ready to give the info
                char* buffer = (char*)waiting_task->SavedRegisters.rbx;
                size_t buffer_len = waiting_task->SavedRegisters.rcx;

                uint64_t prev_pml4 = PML4_Physical;
                mem_set_cr3(waiting_task->Base_PML4, false);
                if(key_buffer_pos > buffer_len){
                    memcpy(buffer, &key_buffer, buffer_len);
                }else{
                    memcpy(buffer, &key_buffer, key_buffer_pos);
                }

                memset(key_buffer, 0, 256);
                key_buffer_pos = 0;

                waiting_task->WaitingReason = WAITING_REASON_NULL;
                waiting_task->ProcessState = READY_PROCESS_STATE;
                mem_set_cr3(prev_pml4, true);
            }
        }
    }

    pic_send_eoi(0x01);
}
#include "LowLevel/Timer.h"

#define BASE_FREQUENCY 1193180

uint64_t SecondsSinceBoot = 0;
uint64_t BiosTime = 0;
uint64_t TimerWindow = 0;
uint16_t Frequency = 1193180;

bool enabled = false;

bool tasks_enabled = false;

bool first = true;

void task_switch_frame(InterruptRegisters* dest, InterruptRegisters* src){
    memcpy((void*)dest, (void*)src, sizeof(InterruptRegisters));
}

/**
 * @brief Switches the current PID to the target, also sets the CR3.
 */
void task_switch(int pid){
    Task* task = &TaskManager[pid];
    
    TASKMGR_set_current(pid);

    uint64_t TASK_CR3 = task->Base_PML4;
    mem_set_cr3(TASK_CR3);
}

int find_next_task(int cur_pid){
    for(int i = cur_pid+1; i < 512; i++){
        if(TaskManager[i].Exists == true){
            return i;
        }
    }
    return 0;
}

bool IsLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

DateData ParseSeconds(uint64_t time){
    DateData ret;

    ret.second = (uint16_t)(time % 60);
    time /= 60;
    ret.minute = (uint16_t)(time % 60);
    time /= 60;
    ret.hour = (uint16_t)(time % 60);
    time /= 24;
    ret.day = (uint16_t)time;

    return ret;
}

void PrintCycles(){
    print("[", 0);
    char cycles[36];
    int_to_char_array(TimerWindow, cycles, sizeof(cycles), 10);
    print(cycles, 0);
    print("]", 0);
}

void PrintSecondsSinceBoot(){
    print("[", 0);
    char cycles[36];
    int_to_char_array(SecondsSinceBoot, cycles, sizeof(cycles), 10);
    print(cycles, 0);
    print("]", 0);
}

void TimerInterrupt(InterruptRegisters* frame){    
    TimerWindow++;
    
    TimerWindow %= 1000;
    if(TimerWindow == 999){SecondsSinceBoot++;}

    if(!tasks_enabled){
        task_switch_frame(&TaskManager[0].SavedRegisters, frame);
    }

    if(tasks_enabled){
        int cur_pid = TASKMGR_get_current();

        // update cur tasks regs
        task_switch_frame(&TaskManager[cur_pid].SavedRegisters, frame);

        if(TaskManager[cur_pid].UsedTicks >= TaskManager[cur_pid].MaxTicks || TaskManager[cur_pid].ProcessState == KILL_PROCESS_STATE){
            TaskManager[cur_pid].UsedTicks = 0;
            if(TaskManager[cur_pid].ProcessState == KILL_PROCESS_STATE){
                TaskManager[cur_pid].ProcessState = NULL_PROCESS_STATE;
                TaskManager[cur_pid].Exists = false;
            }
            int next_pid = find_next_task(cur_pid);

            if(next_pid == 512){ 
                pic_send_eoi(0x00); 
                return; 
            }

            // make it so we return with the correct next task's info
            task_switch_frame(frame, &TaskManager[next_pid].SavedRegisters);
            
            task_switch(next_pid);
        }else{
            TaskManager[cur_pid].UsedTicks++;
        }
    }

    pic_send_eoi(0x00);
}

void SetTimerFrequency(uint16_t hz) {
    uint16_t divisor = (uint16_t)(BASE_FREQUENCY / hz);
    
    Frequency = hz;

    // Channel 0, Mode 3 (Square Wave), LSB/MSB
    outb(0x43, 0x36); 
    outb(0x40, divisor & 0xFF); // Low byte
    outb(0x40, (divisor >> 8) & 0xFF);   // High byte
}

void SyncTime(InterruptRegisters* frame){
    BiosTime = 0;
    SecondsSinceBoot = 0;
    
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;

    outb(0x70, (1 << 7) | 0x00);
    seconds = inb(0x71);

    outb(0x70, (1 << 7) | 0x02);
    minutes = inb(0x71);

    outb(0x70, (1 << 7) | 0x04);
    hours = inb(0x71);

    outb(0x70, (1 << 7) | 0x07);
    day = inb(0x71);


    BiosTime+=(uint64_t)seconds;
    BiosTime+=(uint64_t)minutes*60;
    BiosTime+=(uint64_t)hours*(60*60);
    BiosTime+=(uint64_t)day*86400;

    pic_send_eoi(0x08);
    pic_mask(0x08);
    pic_unmask(0x00);
}

void ForceSwitch(InterruptRegisters* frame){
    int cur_pid = TASKMGR_get_current();

    TaskManager[cur_pid].UsedTicks = 0;
    if(TaskManager[cur_pid].ProcessState == KILL_PROCESS_STATE){
        TaskManager[cur_pid].ProcessState = NULL_PROCESS_STATE;
        TaskManager[cur_pid].Exists = false;
    }
    int next_pid = find_next_task(cur_pid);

    if(next_pid == 512){ 
        pic_send_eoi(0x00); 
        return; 
    }

    // make it so we return with the correct next task's info
    task_switch_frame(frame, &TaskManager[next_pid].SavedRegisters);
            
    task_switch(next_pid);
}
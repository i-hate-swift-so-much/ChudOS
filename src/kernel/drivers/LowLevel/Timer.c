#include "LowLevel/Timer.h"

#define BASE_FREQUENCY 1193180

volatile uint64_t SecondsSinceBoot = 0;
volatile uint64_t BiosTime = 0;
volatile uint64_t TimerWindow = 0;
volatile uint16_t Frequency = 1193180;

volatile bool tasks_enabled;

volatile uint64_t die;

void task_switch_frame(InterruptRegisters* dest, InterruptRegisters* src){
    barrier();
    memcpy((void*)dest, (void*)src, sizeof(InterruptRegisters));
}

/**
 * @brief Switches the current PID to the target, also sets the CR3 and TSS
 */
void task_switch(int pid, InterruptRegisters* frame){
    barrier();
    int cur = TASKMGR_get_current();
    volatile Task* old_task = (volatile Task*)&TaskManager[cur];
    task_switch_frame(&old_task->SavedRegisters, frame);

    volatile Task* task = (volatile Task*)&TaskManager[pid];
    
    TASKMGR_set_current(pid);

    asm volatile("mov %0, %%dr0" : : "r"((uint64_t)pid) : "memory");

    uint64_t TASK_CR3 = task->Base_PML4;
    mem_set_cr3(TASK_CR3, true); // update cr3
    task_switch_frame(frame, &task->SavedRegisters); // update return info
    UpdateActiveTSS(&task->UserTSS); // update tss
}

int find_next_task(int cur_pid){
    if(cur_pid == 511){return 0;}
    for(int i = cur_pid+1; i < 512; i++){
        if(TaskManager[i].Exists == true && TaskManager[i].ProcessState == READY_PROCESS_STATE){
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

void EnableTasks(){
    pic_mask(0x00);
    tasks_enabled = true;
    printf("Tasks Enabled!\n");
    __asm__ __volatile__("mfence" ::: "memory");
    pic_unmask(0x00);
    barrier();
}

void TimerInterrupt(InterruptRegisters* frame){  
    TimerWindow++;

    int cur_pid = TASKMGR_get_current();

    volatile Task* cur_task = (volatile Task*)&TaskManager[cur_pid];

    if(cur_task->ProcessState == CREATION_PROCESS_STATE){
        pic_send_eoi(0x00);
        return;
    }

    // update cur tasks regs
    task_switch_frame(&cur_task->SavedRegisters, frame);

    if(cur_task->UsedTicks >= cur_task->MaxTicks || cur_task->ProcessState == KILL_PROCESS_STATE || cur_task->ProcessState == WAITING_PROCESS_STATE){
        barrier();

        cur_task->UsedTicks = 0;
        if(cur_task->ProcessState == KILL_PROCESS_STATE){
            cur_task->ProcessState = NULL_PROCESS_STATE;
            cur_task->Exists = false;
            free_task_memory(cur_pid);
        }
        int next_pid = find_next_task(cur_pid);

        volatile Task* NextTask = (volatile Task*)&TaskManager[next_pid];

        task_switch(next_pid, frame);
    }else{
        cur_task->UsedTicks++;
    }

    asm volatile("mov %0, %%dr0" : : "r"((uint64_t)TASKMGR_get_current()) : "memory");

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
        memset(&TaskManager[cur_pid], 0, sizeof(Task));
    }

    int next_pid = find_next_task(cur_pid);

    //printf("FORCE SWITCH %i->%i\n", cur_pid, next_pid);
    
    task_switch_frame(&TaskManager[cur_pid].SavedRegisters, frame);
            
    task_switch(next_pid, frame);
}
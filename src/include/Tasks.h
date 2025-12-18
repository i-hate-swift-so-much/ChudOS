#pragma once
#include "Memory.h"

// Defines how many cycles each level of priority can use
#define SUDO_PRIORITY 40
#define USER_PRIORITY 20
#define LOW_PRIORITY 10

// Defines the type of states a process can be in
// used when setting Task.ProcessState
#define NULL_PROCESS_STATE 0x00 // this is used because when the list of processes is enabled, all entries are just zeros. If this was anything else the scheduler would read it as valid even though it isn't initialized
#define NEW_PROCESS_STATE 0x01 // process was just created and it's memory is being allocated
#define READY_PROCESS_STATE 0x02 // process is ready to be ran by the cpu
#define RUNNING_PROCESS_STATE 0x03 // process is currently being run by the cpu. Only one process can be running at a time, if multiple are then a kernel panic will be triggered
#define WAITING_PROCESS_STATE 0x04 // process is waiting in one of the waiting queues
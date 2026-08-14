#include "Userland/terminal.h"

int screenOwner = 0;

bool TTY = true;

bool requestScreenOwnership(int pid){
    if(screenOwner == 0){ screenOwner = pid; return true; }
    return false;
}

void relinquishScreenOwnership(int pid){
    if(screenOwner == pid){ screenOwner = 0; }
}

void setTTY(int pid, bool new){
    if(screenOwner == pid){ TTY = new; }
}
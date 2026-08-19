#include "Userland/terminal.h"

int screenOwner = 0;

bool TTY = true;
struct Terminal terminals[8];

void initVirtualTerminals(bool debug){
    memset(terminals, 0, sizeof(struct Terminal) * 8);

    struct Terminal* kernelTerminal = &terminals[0];
    struct Terminal* userTerminal = &terminals[1];

    if(debug){
        kernelTerminal->visible = true;
        kernelTerminal->w = VGA_WIDTH / 2;
        kernelTerminal->x = VGA_WIDTH / 2;
        kernelTerminal->y = 0;

        userTerminal->visible = true;
        userTerminal->w = VGA_WIDTH / 2;
        userTerminal->x = 0;
        userTerminal->y = 0;
    }else{
        kernelTerminal->visible = false;
        kernelTerminal->w = VGA_WIDTH;
        kernelTerminal->x = 0;
        kernelTerminal->y = 0;

        userTerminal->visible = true;
        userTerminal->w = VGA_WIDTH;
        userTerminal->x = 0;
        userTerminal->y = 0;
    }

    memcpy(&kernelTerminal->name, "KERNEL", 6);
    kernelTerminal->nameLen = 6;
    kernelTerminal->bg = WHITE;
    kernelTerminal->fg = BLACK;

    memcpy(&userTerminal->name, "USER", 6);
    userTerminal->nameLen = 4;
    userTerminal->bg = BLACK;
    userTerminal->fg = WHITE;

    kernelTerminal->h = VGA_HEIGHT;
    userTerminal->h = VGA_HEIGHT;
}

void refreshTerminals(){
    ClearScreen();

    for(int i = 0; i < 8; i++){
        struct Terminal* cur = &terminals[i];
        if(cur->visible){
            // if the terminal is meant to be visible, print in it's pos.
            int curX = cur->x;
            int curY = cur->y+1;
            int curI = 0;
            int skip = 0;
            SetTextColor(~cur->fg & 0xF, ~cur->bg & 0xF);
            for(int n = 0; n < cur->w; n++){
                int nx = cur->x+n;
                if(nx <= VGA_WIDTH && n < cur->nameLen){
                    WriteCharacter(cur->name[n], nx, cur->y);
                }else if(nx <= VGA_WIDTH){
                    WriteCharacter(' ', nx, cur->y);
                }
            }
            SetTextColor(cur->fg, cur->bg);
            int tlen = 0;

            int firstnewline = 0;

            for(int y = 0; y < cur->h; y++){
                for(int x = 0; x < cur->w; x++){
                    char curC = cur->bytes[curI];
                    if(curY > VGA_HEIGHT || curX > VGA_WIDTH){
                        curX++;
                    }else if(skip > 1){
                        skip--;
                        WriteCharacter('\0', curX, curY);
                        curX++;
                        tlen++;
                    }else if(curC == '\n'){
                        int diff = (cur->w) - x;
                        skip = diff;
                        curI++;
                        tlen+=diff;
                        if(firstnewline == 0){
                            firstnewline = curI;
                        }
                    }else if(curC == '\b'){
                        x--;
                        curX--;
                        curI++;
                        tlen--;
                    }else if(curC == '\t'){
                        skip+=4;
                        curI++;
                        tlen+=4;
                    }else if(curC == '\r'){
                        
                    }else if (curC == '\0'){
                        WriteCharacter(curC, curX, curY);
                        curX++;
                        curI++;
                    }else{
                        WriteCharacter(curC, curX, curY);
                        curX++;
                        curI++;
                        tlen++;
                    }

                    if(tlen > (cur->w * cur->h)){
                        cur->lastPos = 0;
                        // delete first line
                        memcpy(cur->bytes, &cur->bytes[firstnewline], 2000-firstnewline);
                    }
                }
                curX = cur->x;
                curY++;
            }
        }
    }
}

void virtualprint(int tid, char* src){
    int len = 0;
    char cur;
    while(len < 128){
        cur = src[len];
        if(cur == '\0'){
            break;
        }
        len++;
    }

    struct Terminal* term = (struct Terminal*)&terminals[tid];

    bool nadd = term->lastPos + len > (term->w * term->h);

    if(nadd){
        term->lastPos = 0;
    }

    memcpy(&term->bytes[term->lastPos], src, len);

    if(!nadd){
        term->lastPos += len;
    }

    refreshTerminals();
}

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
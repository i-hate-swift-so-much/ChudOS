#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#include "ErrorHandling/ErrorCodes.h"

enum ERR_CLASS{
    MEMORY,
    DISK,
    
};

enum ERR_SUBCLASS{
    MEMORY_INVALD = 1, // when the accessed memory address is not within the processes memory space (RESOLVED or KILL)
    MEMORY_OUT_OF_SPACE = 2, // when an attempt is made to alloc, but there is not enough physical space (KILL or PANIC)
    MEMORY_PAGE_FAULT = 3, // when the above conditions are not met and it is just a page fault
}; 

enum ERR_SUB1{
    t
};

enum ERR_SUB2{
    q
};

enum ERR_SUB3{
    e
};

enum ERR_SUB4{
    f
};

enum ERR_SEVERITY{
    SEV_RECOVERED,
    SEV_WARNING,
    SEV_CAUTION,
    SEV_KILL,
    SEV_PANIC
};

// error code structure (48 bits)
// Top 4 bits = enum ERR_CLASS
// Next 4 bits = enum ERR_SUBCLASS
// Next 16 bits = PID
// Next 16 bits = (ERR_SUB1 << 12) || (ERR_SUB2 << 8) || (ERR_SUB3 << 4) || (ERR_SUB4)
// Next 4 bits = enum ERR_SEVERITY

void PrintErrorCode(uint64_t code);
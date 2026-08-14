#pragma once

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#include "Tasks.h"

extern int screenOwner;

bool requestScreenOwnership(int pid);
void relinquishScreenOwnership(int pid);
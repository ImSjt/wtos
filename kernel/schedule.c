#include "global.h"
#include "proc.h"
#include "string.h"

void schedule()
{
    
}

void scheduleTick()
{
    ++procReady;
    if(procReady >= procTable+NR_TASKS)
        procReady = procTable;
}


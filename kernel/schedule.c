#include "global.h"
#include "proc.h"
#include "string.h"

void schedule()
{
    
}

void scheduleTick(int n)
{
    /* 中断重入 */
    if(kreenter != 0)
    {
        dispStr("!");
        return;
    }

    ++ticks;
    ++procReady;
    if(procReady >= procTable+NR_TASKS)
        procReady = procTable;
}


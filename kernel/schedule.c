#include "global.h"
#include "proc.h"
#include "string.h"

void schedule()
{
    int tick = 0;
    while(tick <= 0)
    {
        int i;
        for(i = 0; i < NR_TASKS; ++i)
        {
            if(tick < procTable[i].ticks)
            {
                procReady = &procTable[i];
                tick = procTable[i].ticks;
            }
        }

        if(tick <= 0)
        {
            for(i = 0; i < NR_TASKS; ++i)
            {
                procTable[i].ticks = procTable[i].priority;
            }
        }
    }
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

    if(procReady->ticks > 0)
    {
        --procReady->ticks;
        return;
    }

    schedule();
}


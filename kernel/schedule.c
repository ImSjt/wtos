#include "global.h"
#include "proc.h"
#include "string.h"
#include "stdio.h"

void schedule()
{
    int tick = 0;
    while(tick <= 0)
    {
        int i;
        for(i = 0; i < NR_TOTAL_PROCS; ++i)
        {
            if(procTable[i].flag == 0)
            {
                if(tick < procTable[i].ticks)
                {
                    procReady = &procTable[i];
                    tick = procTable[i].ticks;
                }
            }
        }

        if(tick <= 0)
        {
            for(i = 0; i < NR_TOTAL_PROCS; ++i)
            {
                if(procTable[i].flag == 0)
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
        //dispStr("!");
        return;
    }

	if (keyPressed)
	{
        informInt(P_TASK_TTY);
    }

    ++ticks;
    if(procReady->ticks > 0)
    {
        --procReady->ticks;
        return;
    }

    schedule();
}


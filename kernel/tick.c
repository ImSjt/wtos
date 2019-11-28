#include "string.h"
#include "global.h"
#include "syscall.h"
#include "irq.h"

int sysGetTicks()
{
    return ticks;
}

void mdelay(int ms)
{
    int t = getTicks();   

    while((getTicks()-t)*1000/HZ < ms);
}

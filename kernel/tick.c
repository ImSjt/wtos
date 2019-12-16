#include "string.h"
#include "global.h"
#include "syscall.h"
#include "irq.h"
#include "protect.h"
#include "type.h"
#include "systask.h"
#include "ipc.h"
#include "proc.h"

int getTicks()
{
    struct Message msg;
    memset(&msg, 0, sizeof(msg));

    msg.type = GET_TICKS;
    ipc(BOTH, P_SYSTASK, &msg);

    return msg.RETVAL;
}

void mdelay(int ms)
{
    int t = getTicks();

    while((getTicks()-t)*1000/HZ < ms)
    {}
}

void initTime()
{
    /* 初始化定时器，使其10ms中断一次 */
    outByte(TIMER_MODE, RATE_GENERATOR);
    outByte(TIMER0, (u8)(TIMER_FREQ/HZ));
    outByte(TIMER0, (u8)((TIMER_FREQ/HZ)>>8));

    putIrqHandler(CLOCK_IRQ, scheduleTick);
    enableIrq(CLOCK_IRQ);

}

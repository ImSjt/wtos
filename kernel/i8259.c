#include "type.h"
#include "irq.h"
#include "protect.h"
#include "string.h"
#include "global.h"

void doIrq(int irq);

/* 初始化中断控制器 */
void init8259A()
{
    /* Master 8259, ICW1. */
    outByte(INT_M_CTL, 0x11);

    /* Slave  8259, ICW1. */
    outByte(INT_S_CTL,	0x11);

    /* Master 8259, ICW2. 设置 '主8259' 的中断入口地址为 0x20. */
    outByte(INT_M_CTLMASK, INT_VECTOR_IRQ0);

    /* Slave  8259, ICW2. 设置 '从8259' 的中断入口地址为 0x28 */
    outByte(INT_S_CTLMASK,	INT_VECTOR_IRQ8);

    /* Master 8259, ICW3. IR2 对应 '从8259'. */
    outByte(INT_M_CTLMASK,	0x4);

    /* Slave  8259, ICW3. 对应 '主8259' 的 IR2. */
    outByte(INT_S_CTLMASK,	0x2);

	/* Master 8259, ICW4. */
	outByte(INT_M_CTLMASK,	0x1);

	/* Slave  8259, ICW4. */
	outByte(INT_S_CTLMASK,	0x1);

	/* Master 8259, OCW1.  */
	outByte(INT_M_CTLMASK,	0xFE);  /* 打开了时钟中断 */

	/* Slave  8259, OCW1.  */
	outByte(INT_S_CTLMASK,	0xFF);

    int i;
    for(i = 0; i < NR_IRQ; ++i)
        irqTable[i] = doIrq;
}

/* 外部中断处理函数 */
void doIrq(int irq)
{
    dispStr("do irq: ");
    dispInt(irq);
    dispStr("\n");
}

void putIrqHandler(int irq, irqHandler handler)
{
	disableIrq(irq);
	irqTable[irq] = handler;
}


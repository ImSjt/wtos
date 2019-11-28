#ifndef _IRQ_H_
#define _IRQ_H_
#include "type.h"

#define TIMER0 0x40 /* I/O port for timer channel 0 */
#define TIMER_MODE 0x43 /* I/O port for timer mode control */
#define RATE_GENERATOR 0x34 /* 00-11-010-0 : Counter0 - LSB then MSB - rate generator - binary 47 */

#define TIMER_FREQ 1193182L/* clock frequency for timer in PC and AT */
#define HZ 100 /* clock freq (software settable on IBM-PC) */

/* 写端口 */
void outByte(u16 port, u8 value);

/* 读端口 */
u8 inByte(u16 port);

void init8259A();

void disableIrq(int irq);
void enableIrq(int irq);

void putIrqHandler(int irq, irqHandler handler);

void scheduleTick(int n);

void mdelay(int ms);

#endif /* _IRQ_H_ */

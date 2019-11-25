#ifndef _IRQ_H_
#define _IRQ_H_
#include "type.h"

/* 写端口 */
void outByte(u16 port, u8 value);

/* 读端口 */
u8 inByte(u16 port);

void init8259A();

void disableIrq(int irq);
void enableIrq(int irq);

void putIrqHandler(int irq, irqHandler handler);

void scheduleTick(int n);


#endif /* _IRQ_H_ */

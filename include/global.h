#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#include "type.h"
#include "protect.h"
#include "proc.h"

extern u8 gdtPtr[6];                       /* 0~15:Limit  16~47:Base */
extern struct SegmentDescriptor gdt[GDT_SIZE];    /* GDT */

extern u8 idtPtr[6];
extern struct Gate idt[IDT_SIZE];

extern int dispPos;

extern struct Process procTable[];
extern struct Process* procReady;

extern struct TSS tss;

extern int kreenter;

extern irqHandler irqTable[];

#endif /* _GLOBAL_H_ */

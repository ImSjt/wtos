#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"

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

extern systemCall sysCallTable[];

extern int ticks;

extern struct TTY ttyTable[];
extern struct Console consoleTable[];
extern int curConsole;

extern struct DevDrvMap ddmap[];

extern u8*		    fsbuf;
extern const int	FSBUF_SIZE;

#endif /* _GLOBAL_H_ */

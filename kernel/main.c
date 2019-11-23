#include "type.h"
#include "protect.h"
#include "string.h"
#include "irq.h"
#include "proc.h"

/* GDT */
u8 gdtPtr[6];                       /* 0~15:Limit  16~47:Base */
struct SegmentDescriptor gdt[GDT_SIZE];    /* GDT */

/* LDT */
u8 idtPtr[6];
struct Gate idt[IDT_SIZE];

int dispPos;

/* 进程描述表 */
struct Process procTable[NR_TASKS];
struct Process* procReady;

/* 任务栈 */
char taskStack[STACK_SIZE_TOTAL];

/* TSS */
struct TSS tss;

extern void restart();

static void testA();

void kinit()
{
    dispPos = 0;
    dispStr("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
        "-----\"kinit\"begins-----\n");

    memcpy(&gdt,
            (void*)(*(u32*)(&gdtPtr[2])),
            *((u16*)(&gdtPtr[0]))+1);

    u16* gdtLimit = (u16*)(&gdtPtr[0]);
    u32* gdtBase = (u32*)(&gdtPtr[2]);
    *gdtLimit = GDT_SIZE * sizeof(struct SegmentDescriptor) - 1;
    *gdtBase = (u32)(&gdt);

    u16* idtLimit = (u16*)(&idtPtr[0]);
    u32* idtBase = (u32*)(&idtPtr[2]);
    *idtLimit = IDT_SIZE * sizeof(struct Gate) - 1;
    *idtBase = (u32)(&idt);

    initProt();

    dispStr("-----\"kinit\" ends-----\n");
}

int kmain()
{
    dispStr("-----\"kernel_main\"begins-----\n");

    struct Process* proc = procTable;

    /* 段选择子，LDT在GDT中的位置 */
    proc->ldtSelector = SELECTOR_LDT_FIRST;

    /* 初始化代码段 */
    memcpy(&proc->ldts[0], &gdt[SELECTOR_KERNEL_CS>>3],
                sizeof(struct SegmentDescriptor));
    proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
    
    /* 初始化数据段 */
    memcpy(&proc->ldts[1], &gdt[SELECTOR_KERNEL_DS>>3],
                sizeof(struct SegmentDescriptor));
    proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;

    /* 初始化寄存器的值，cs指向LDT中第一个段描述符，其它指向第二个 */
	proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;
	proc->regs.eip= (u32)testA; /* 进程入口地址 */
	proc->regs.esp= (u32)taskStack + STACK_SIZE_TOTAL;
	proc->regs.eflags = 0x1202;	/* IF=1, IOPL=1, bit 2 is always 1.iret后，会打开中断和设置IO允许位 */

    procReady = proc;
    restart();

    while(1)
    {

    }

    return 0;
}

/* 进程A */
static void testA()
{
    int i = 0;

    while(1)
    {
        dispStr("A");
        dispInt(i++);
        dispStr(".");
        delay(10);
    }
}
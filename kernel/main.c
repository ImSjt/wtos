#include "type.h"
#include "protect.h"
#include "string.h"
#include "irq.h"
#include "proc.h"
#include "syscall.h"
#include "proto.h"
#include "stdio.h"
#include "ipc.h"
#include "systask.h"
#include "fs.h"
#include "hd.h"

/* -----------------------------全局变量-----------------------------*/
/* GDT，全局段描述符表 */
u8 gdtPtr[6];                       /* 0~15:Limit  16~47:Base */
struct SegmentDescriptor gdt[GDT_SIZE];    /* GDT */

/* LDT，中断向量表 */
u8 idtPtr[6];
struct Gate idt[IDT_SIZE];

/* 用于显示字符，暂且这么用 */
int dispPos;

/* 进程描述表 */
struct Process procTable[NR_TOTAL_PROCS]; /* 进程描述表 */
struct Process* procReady;  /* 准备就绪的进程 */

/* TSS */
struct TSS tss;

/* 控制中断可重入 */
int kreenter;

/* 中断处理函数 */
irqHandler irqTable[NR_IRQ];

/* 系统调用 */
systemCall sysCallTable[NR_SYS_CALL] = {
    sysPrintx,
    sysSendrec
};

int ticks;

/* 主设备号为下标，驱动程序号为值 */
struct DevDrvMap ddmap[] = {
	/* driver nr.		major device nr.
	   ----------		---------------- */
	{INVALID_DRIVER},	/**< 0 : Unused */
	{INVALID_DRIVER},	/**< 1 : Reserved for floppy driver */
	{INVALID_DRIVER},	/**< 2 : Reserved for cdrom driver */
	{P_TASK_HD},		/**< 3 : Hard disk */
	{P_TASK_TTY},		/**< 4 : TTY */
	{INVALID_DRIVER}	/**< 5 : Reserved for scsi disk driver */
};

/**
 * 6MB~7MB: buffer for FS
 */
u8*		    fsbuf		= (u8*)0x600000;
const int	FSBUF_SIZE	= 0x100000;

/* fs */
struct FileDesc     fDescTable[NR_FILE_DESC];
struct Inode		inodeTable[NR_INODE];
struct SuperBlock	superBlock[NR_SUPER_BLOCK];
struct Message  fsMsg;
struct Process* pcaller;
struct Inode*	rootInode;
/* ------------------------------------------------------------------*/

extern void restart();

static void testA();
static void testB();
static void testC();

void kinit()
{
    dispPos = 0;
    kreenter = 0;
    ticks = 0;
    
    /* 将GDT拷贝到新地址处 */
    memcpy(&gdt, (void*)(*(u32*)(&gdtPtr[2])),  *((u16*)(&gdtPtr[0]))+1);

    /* 更新GDT的位置 */
    u16* gdtLimit = (u16*)(&gdtPtr[0]);
    u32* gdtBase = (u32*)(&gdtPtr[2]);
    *gdtLimit = GDT_SIZE * sizeof(struct SegmentDescriptor) - 1;
    *gdtBase = (u32)(&gdt);

    /* 更新中断向量表的位置 */
    u16* idtLimit = (u16*)(&idtPtr[0]);
    u32* idtBase = (u32*)(&idtPtr[2]);
    *idtLimit = IDT_SIZE * sizeof(struct Gate) - 1;
    *idtBase = (u32)(&idt);

    initProt();
}

static void reset()
{
    pcaller = 0;
}

int kmain()
{
    dispStr("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n----start kernel----\n");

    struct Process* proc = procTable;
    int i;

    reset();

    /* 内核进程 */
    for(i = 0; i < NR_TASKS; ++i)
    {
        memset(&proc[i], 0, sizeof(proc[i]));
        proc[i].pid = i;
        proc[i].regs.cs = SELECTOR_TASK_CS;
        proc[i].regs.ds = SELECTOR_TASK_DS;
        proc[i].regs.es = SELECTOR_TASK_DS;
        proc[i].regs.fs = SELECTOR_TASK_DS;
        proc[i].regs.ss = SELECTOR_TASK_DS;
        proc[i].regs.gs = SELECTOR_GS;
        proc[i].regs.esp = (u32)proc[i].stack + STACK_SIZE;
        proc[i].regs.eflags = 0x1202;	/* IF=1, IOPL=1, bit 2 is always 1.iret后 */
        proc[i].ticks = 0;
        proc[i].flag = 0;
        proc[i].msg = 0;
        proc[i].sendto = NO_TASK;
        proc[i].recvfrom = NO_TASK;
        proc[i].hasIntMsg = 0;
        proc[i].sending = 0;
        proc[i].nextSending = 0;
    }

    /* 内核进程 */    
    proc[P_TASK_TTY].regs.eip = (u32)taskTTY;
    proc[P_SYSTASK].regs.eip = (u32)taskSys;
    proc[P_TASK_HD].regs.eip = (u32)taskHd;
    proc[P_TASK_FS].regs.eip = (u32)taskFs;

    proc[P_TASK_TTY].priority = 15;
    proc[P_SYSTASK].priority = 10;
    proc[P_TASK_HD].priority = 10;
    proc[P_TASK_FS].priority = 10;

    proc[P_TASK_TTY].tty = 0;
    proc[P_SYSTASK].tty = 0;
    proc[P_TASK_HD].tty = 0;
    proc[P_TASK_FS].tty = 0;

    /* 用户进程 */
    for(i = NR_TASKS; i < NR_TOTAL_PROCS; ++i)
    {
        memset(&proc[i], 0, sizeof(proc[i]));
        proc[i].pid = i;
        proc[i].regs.cs = SELECTOR_USER_CS;
        proc[i].regs.ds = SELECTOR_USER_DS;
        proc[i].regs.es = SELECTOR_USER_DS;
        proc[i].regs.fs = SELECTOR_USER_DS;
        proc[i].regs.ss = SELECTOR_USER_DS;
        proc[i].regs.gs = SELECTOR_GS;
        proc[i].regs.esp = (u32)proc[i].stack + STACK_SIZE;
        proc[i].regs.eflags = 0x1202;	/* IF=1, IOPL=1, bit 2 is always 1.iret后 */
        proc[i].ticks = 0;
        proc[i].flag = 0;
        proc[i].msg = 0;
        proc[i].sendto = NO_TASK;
        proc[i].recvfrom = NO_TASK;
        proc[i].hasIntMsg = 0;        
        proc[i].sending = 0;
        proc[i].nextSending = 0;
    }
    
    proc[NR_TASKS].regs.eip = (u32)testA;
    proc[NR_TASKS+1].regs.eip = (u32)testB;

    proc[NR_TASKS].priority = 5;
    proc[NR_TASKS+1].priority = 5;

    proc[NR_TASKS].tty = 0;
    proc[NR_TASKS+1].tty = 0;

    procReady = proc; /* 设置下一个调度的进程 */

    initTime();

    restart(); /* 启动第一个进程 */

    while(1)
    {}

    return 0;
}

/* 进程A */
static void testA()
{
    int fd = open("/blah", O_CREAT);
    printf("fd:%d\n", fd);
    close(fd);

    while(1)
    {
        mdelay(1000);
        printf("A ");
    }
}

/* 进程B */
static void testB()
{
    while(1)
    {
        mdelay(1000);
        printf("B ");
    }
}

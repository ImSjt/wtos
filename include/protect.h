#ifndef	_PROTECT_H_
#define	_PROTECT_H_
#include "type.h"

/* 存储段描述符/系统段描述符 */
struct SegmentDescriptor		/* 共 8 个字节 */
{
	u16	limitLow;		/* Limit */
	u16	baseLow;		/* Base */
	u8	baseMid;		/* Base */
	u8	attr1;			/* P(1) DPL(2) DT(1) TYPE(4) */
	u8	limitHighAttr2;	/* G(1) D(1) 0(1) AVL(1) LimitHigh(4) */
	u8	baseHigh;		/* Base */
};

struct Gate
{
	u16	offsetLow;	/* Offset Low */
	u16	selector;	/* Selector */
	u8	dcount;		/* 该字段只在调用门描述符中有效。如果在利用
				   调用门调用子程序时引起特权级的转换和堆栈
				   的改变，需要将外层堆栈中的参数复制到内层
				   堆栈。该双字计数字段就是用于说明这种情况
				   发生时，要复制的双字参数的数量。*/
	u8	attr;		/* P(1) DPL(2) DT(1) TYPE(4) */
	u16	offsetHigh;	/* Offset High */
};

struct TSS
{
	u32	backlink;
	u32	esp0;	/* stack pointer to use during interrupt */
	u32	ss0;	/*   "   segment  "  "    "        "     */
	u32	esp1;
	u32	ss1;
	u32	esp2;
	u32	ss2;
	u32	cr3;
	u32	eip;
	u32	flags;
	u32	eax;
	u32	ecx;
	u32	edx;
	u32	ebx;
	u32	esp;
	u32	ebp;
	u32	esi;
	u32	edi;
	u32	es;
	u32	cs;
	u32	ss;
	u32	ds;
	u32	fs;
	u32	gs;
	u32	ldt;
	u16	trap;
	u16	iobase;	/* I/O位图基址大于或等于TSS段界限，就表示没有I/O许可位图 */
};

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	INDEX_MAX
#define IDT_SIZE	256 /* 中断向量 */

/* 每个进程的LDT的个数 */
#define LDT_SIZE 2

/* GDT */
/* 描述符索引 */
#define	INDEX_DUMMY			0
#define INDEX_KERNEL_CS     1
#define INDEX_KERNEL_DS     2
#define INDEX_USER_CS       3
#define INDEX_USER_DS       4
#define INDEX_VIDEO         5
#define INDEX_TSS           6
#define INDEX_MAX           (INDEX_TSS+1)

/* 选择子 */
#define OFFSET_INDEX            3
#define	SELECTOR_DUMMY		    (INDEX_DUMMY << OFFSET_INDEX)
#define	SELECTOR_KERNEL_CS		(INDEX_KERNEL_CS << OFFSET_INDEX)   /* 内核代码段 */
#define SELECTOR_KERNEL_DS      (INDEX_KERNEL_DS << OFFSET_INDEX)   /* 内核数据段 */
#define SELECTOR_USER_CS        ((INDEX_USER_CS << OFFSET_INDEX) | RPL_USER) /* 用户代码段 */
#define SELECTOR_USER_DS        ((INDEX_USER_DS << OFFSET_INDEX) | RPL_USER) /* 用户数据段 */
#define SELECTOR_GS             ((INDEX_VIDEO << OFFSET_INDEX) | RPL_USER)
#define SELECTOR_TSS            (INDEX_TSS << OFFSET_INDEX)

/* 权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

/* RPL */
#define	RPL_KRNL	SA_RPL0
#define	RPL_TASK	SA_RPL1
#define	RPL_USER	SA_RPL3

/* 选择子类型值说明 */
/* 其中, SA_ : Selector Attribute */
#define	SA_RPL_MASK	0xFFFC
#define	SA_RPL0		0
#define	SA_RPL1		1
#define	SA_RPL2		2
#define	SA_RPL3		3

#define	SA_TI_MASK	0xFFFB
#define	SA_TIG		0
#define	SA_TIL		4

/* 8259A interrupt controller ports. */
#define INT_M_CTL     0x20 /* I/O port for interrupt controller       <Master> */
#define INT_M_CTLMASK 0x21 /* setting bits in this port disables ints <Master> */
#define INT_S_CTL     0xA0 /* I/O port for second interrupt controller<Slave>  */
#define INT_S_CTLMASK 0xA1 /* setting bits in this port disables ints <Slave>  */

/* 描述符类型值说明 */
#define	DA_32			0x4000	/* 32 位段				*/
#define	DA_LIMIT_4K		0x8000	/* 段界限粒度为 4K 字节			*/
#define	DA_DPL0			0x00	/* DPL = 0				*/
#define	DA_DPL1			0x20	/* DPL = 1				*/
#define	DA_DPL2			0x40	/* DPL = 2				*/
#define	DA_DPL3			0x60	/* DPL = 3				*/

/* 存储段描述符类型值说明 */
#define	DA_DR			0x90	/* 存在的只读数据段类型值		    */
#define	DA_DRW			0x92	/* 存在的可读写数据段属性值		    */
#define	DA_DRWA			0x93	/* 存在的已访问可读写数据段类型值	*/
#define	DA_C			0x98	/* 存在的只执行代码段属性值		    */
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_CCO			0x9C	/* 存在的只执行一致代码段属性值	    */
#define	DA_CCOR			0x9E	/* 存在的可执行可读一致代码段属性值	*/

/* 系统段描述符类型值说明 */
#define	DA_LDT			0x82	/* 局部描述符表段类型值		 */
#define	DA_TaskGate		0x85	/* 任务门类型值				*/
#define	DA_386TSS		0x89	/* 可用 386 任务状态段类型值 */
#define	DA_386CGate		0x8C	/* 386 调用门类型值			*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/
#define	DA_386TGate		0x8F	/* 386 陷阱门类型值			*/

/* 中断向量 */
#define	INT_VECTOR_DIVIDE		0x0
#define	INT_VECTOR_DEBUG		0x1
#define	INT_VECTOR_NMI			0x2
#define	INT_VECTOR_BREAKPOINT		0x3
#define	INT_VECTOR_OVERFLOW		0x4
#define	INT_VECTOR_BOUNDS		0x5
#define	INT_VECTOR_INVAL_OP		0x6
#define	INT_VECTOR_COPROC_NOT		0x7
#define	INT_VECTOR_DOUBLE_FAULT		0x8
#define	INT_VECTOR_COPROC_SEG		0x9
#define	INT_VECTOR_INVAL_TSS		0xA
#define	INT_VECTOR_SEG_NOT		0xB
#define	INT_VECTOR_STACK_FAULT		0xC
#define	INT_VECTOR_PROTECTION		0xD
#define	INT_VECTOR_PAGE_FAULT		0xE
#define	INT_VECTOR_COPROC_ERR		0x10
#define INT_VECTOR_SYS_CALL 0x90 /* 系统调用 */

/* 中断向量 */
#define	INT_VECTOR_IRQ0			0x20
#define	INT_VECTOR_IRQ8			0x28

/* Hardware interrupts */
#define	NR_IRQ		16	/* Number of IRQs */
#define	CLOCK_IRQ	0
#define	KEYBOARD_IRQ	1
#define	CASCADE_IRQ	2	/* cascade enable for 2nd AT controller */
#define	ETHER_IRQ	3	/* default ethernet interrupt vector */
#define	SECONDARY_IRQ	3	/* RS232 interrupt vector for port 2 */
#define	RS232_IRQ	4	/* RS232 interrupt vector for port 1 */
#define	XT_WINI_IRQ	5	/* xt winchester */
#define	FLOPPY_IRQ	6	/* floppy disk */
#define	PRINTER_IRQ	7
#define	AT_WINI_IRQ	14	/* at winchester */

#define NR_SYS_CALL 1

/* 虚拟地址转换为物理地址 */
//#define vir2phys(segBase, vir)	(u32)(((u32)segBase) + (u32)(vir))
#define OFFSET_ADDR             0x00000000      // 物理地址到线性地址的偏移值，取决于分页机制决定
#define vir2phys(vir)           (u32)(vir-OFFSET_ADDR)

void initProt();

#endif /* _PROTECT_H_ */

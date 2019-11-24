#include "global.h"
#include "type.h"
#include "string.h"
#include "irq.h"
#include "protect.h"

/* 异常处理函数 */
void divideError();
void singleStepException();
void nmi();
void breakpointException();
void overflow();
void boundsCheck();
void invalOpcode();
void coprNotAvailable();
void doubleFault();
void coprSegOverrun();
void invalTss();
void segmentNotPresent();
void stackException();
void generalProtection();
void pageFault();
void coprError();

/* 中断处理 */
void hwint00();
void hwint01();
void hwint02();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint09();
void hwint10();
void hwint11();
void hwint12();
void hwint13();
void hwint14();
void hwint15();

static void initIdtDesc(unsigned char vector, u8 desc_type,
			  irqHandler handler, unsigned char privilege);

static void initDescriptor(struct SegmentDescriptor* desc, u32 base,
							u32 limit, u16 attribute);

u32 seg2phys(u16 seg)
{
	struct SegmentDescriptor* dest = &gdt[seg >> 3];
	return (dest->baseHigh<<24 | dest->baseMid<<16 | dest->baseLow);
}

void initProt()
{
	/* 初始化中断控制器 */
    init8259A();

	/* 初始化中断向量表 */
	/* 全部初始化成中断门(没有陷阱门) */
	/* 异常 */
	initIdtDesc(INT_VECTOR_DIVIDE, DA_386IGate, divideError, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_DEBUG, DA_386IGate, singleStepException, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_NMI,	DA_386IGate, nmi, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_BREAKPOINT, DA_386IGate, breakpointException, PRIVILEGE_USER);
	initIdtDesc(INT_VECTOR_OVERFLOW, DA_386IGate, overflow, PRIVILEGE_USER);
	initIdtDesc(INT_VECTOR_BOUNDS, DA_386IGate, boundsCheck, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_INVAL_OP, DA_386IGate, invalOpcode, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_COPROC_NOT, DA_386IGate, coprNotAvailable,	PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_DOUBLE_FAULT, DA_386IGate, doubleFault, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_COPROC_SEG, DA_386IGate, coprSegOverrun, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_INVAL_TSS, DA_386IGate, invalTss, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_SEG_NOT,	DA_386IGate, segmentNotPresent, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_STACK_FAULT,	DA_386IGate, stackException, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_PROTECTION, DA_386IGate, generalProtection, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_PAGE_FAULT, DA_386IGate, pageFault, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_COPROC_ERR, DA_386IGate, coprError, PRIVILEGE_KRNL);
	
	/* 外部中断 */
	initIdtDesc(INT_VECTOR_IRQ0+0, DA_386IGate, hwint00, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ0+1, DA_386IGate, hwint01, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ0+2, DA_386IGate, hwint02, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ0+3, DA_386IGate, hwint03, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ0+4, DA_386IGate, hwint04, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ0+5, DA_386IGate, hwint05, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ0+6, DA_386IGate, hwint06, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ0+7, DA_386IGate, hwint07, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+0, DA_386IGate, hwint08, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+1, DA_386IGate, hwint09, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+2, DA_386IGate, hwint10, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+3, DA_386IGate, hwint11, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+4, DA_386IGate, hwint12, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+5, DA_386IGate, hwint13, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+6, DA_386IGate, hwint14, PRIVILEGE_KRNL);
	initIdtDesc(INT_VECTOR_IRQ8+7, DA_386IGate, hwint15, PRIVILEGE_KRNL);


#if 0
	/* 初始化tss的段描述符 */
	memset(&tss, 0, sizeof(tss));
	tss.ss0 = SELECTOR_KERNEL_DS;	/* 暂时只需要使用这一个变量 */
	initDescriptor(&gdt[INDEX_TSS],
					vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
					sizeof(tss)-1,
					DA_386TSS);
	tss.iobase = sizeof(tss); /* 没有I/O许可位图 */

	/* 初始化LDT的段描述符 */
	initDescriptor(&gdt[INDEX_LDT_FIRST], 
					vir2phys(seg2phys(SELECTOR_KERNEL_DS), procTable[0].ldts),
					LDT_SIZE * sizeof(struct SegmentDescriptor)-1,
					DA_LDT);
#endif

    /* 初始化tss，并设置GDT */
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_KERNEL_DS;
    initDescriptor(&gdt[INDEX_TSS], vir2phys(&tss), sizeof(tss)-1, DA_386TSS);
    tss.iobase = sizeof(tss);
}

/* 初始化中断门 */
static void initIdtDesc(unsigned char vector, u8 descType,
			  irqHandler handler, unsigned char privilege)
{
	struct Gate* gate = &idt[vector];
	u32	base = (u32)handler;
	gate->offsetLow	 = base & 0xFFFF;
	gate->selector	 = SELECTOR_KERNEL_CS;
	gate->dcount	 = 0;
	gate->attr		 = descType | (privilege << 5);
	gate->offsetHigh = (base >> 16) & 0xFFFF;
}

/* 异常处理函数 */
void exceptionHandler(int vecNo, int errCode, int eip, int cs, int eflags)
{
    int textColor = 0x74; /* 灰底红字 */

    /* 异常错误提示 */
	char * err_msg[] = {"#DE Divide Error",
			    "#DB RESERVED",
			    "--  NMI Interrupt",
			    "#BP Breakpoint",
			    "#OF Overflow",
			    "#BR BOUND Range Exceeded",
			    "#UD Invalid Opcode (Undefined Opcode)",
			    "#NM Device Not Available (No Math Coprocessor)",
			    "#DF Double Fault",
			    "    Coprocessor Segment Overrun (reserved)",
			    "#TS Invalid TSS",
			    "#NP Segment Not Present",
			    "#SS Stack-Segment Fault",
			    "#GP General Protection",
			    "#PF Page Fault",
			    "--  (Intel reserved. Do not use.)",
			    "#MF x87 FPU Floating-Point Error (Math Fault)",
			    "#AC Alignment Check",
			    "#MC Machine Check",
			    "#XF SIMD Floating-Point Exception"
	};

    for(int i = 0; i < 80*5; ++i)
        dispStr(" ");
    dispPos = 0;

	dispColorStr("VecNo:", textColor);
	dispInt(vecNo);
	dispColorStr("\nException! --> ", textColor);
	dispColorStr(err_msg[vecNo], textColor);
	dispColorStr("\n\n", textColor);
	dispColorStr("EFLAGS:", textColor);
	dispInt(eflags);
	dispColorStr("CS:", textColor);
	dispInt(cs);
	dispColorStr("EIP:", textColor);
	dispInt(eip);

	if(errCode != 0xFFFFFFFF){
		dispColorStr("Error code:", textColor);
		dispInt(errCode);
	}
}

static void initDescriptor(struct SegmentDescriptor* desc, u32 base,
							u32 limit, u16 attribute)
{
	desc->limitLow	= limit & 0x0FFFF;
	desc->baseLow	= base & 0x0FFFF;
	desc->baseMid	= (base >> 16) & 0x0FF;
	desc->attr1		= attribute & 0xFF;
	desc->limitHighAttr2= ((limit>>16) & 0x0F) | (attribute>>8) & 0xF0;
	desc->baseHigh	= (base >> 24) & 0x0FF;
}
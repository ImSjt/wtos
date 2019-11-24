#ifndef _PROC_H_
#define _PROC_H_
#include "type.h"
#include "protect.h"

/* 最大进程个数 */
#define NR_TASKS 1

/* 进程栈大小 */
#define STACK_SIZE_TESTA	0x8000
#define STACK_SIZE_TOTAL	STACK_SIZE_TESTA

struct StackFrame
{
	u32	gs;
	u32	fs;
	u32	es;
	u32	ds;
	u32	edi;
	u32	esi;
	u32	ebp;
	u32	kernelEsp;
	u32	ebx;
	u32	edx;
	u32	ecx;
	u32	eax;
	u32	retaddr;
	u32	eip;
	u32	cs;
	u32	eflags;
	u32	esp;
	u32	ss;
};

/* 进程描述符 */
struct Process
{
	struct StackFrame regs;          /* process registers saved in stack frame */

	u32 pid;                   /* process id passed in from MM */
	char name[16];           /* name of the process */
};

#endif /* _PROC_H_ */
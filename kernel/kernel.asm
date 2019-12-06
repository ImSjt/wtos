%include "const.inc"

; 导入函数
extern kinit
extern exceptionHandler
extern doIrq
extern kmain
extern dispStr
extern scheduleTick

; 导入全局变量
extern gdtPtr       ; GDT
extern idtPtr       ; IDT
extern tss			; TSS
extern procReady	; 下一个进程
extern kreenter
extern irqTable
extern sysCallTable

[section .bss]
StackSpace resb 2*1024  ; 分配2K的栈空间
StackTop:               ; 栈顶

[section .data]
clockMsg db "^", 0

[section .text] ; 代码段

global _start

global restart

; 异常
global divideError
global singleStepException
global nmi
global breakpointException
global overflow
global boundsCheck
global invalOpcode
global coprNotAvailable
global doubleFault
global coprSegOverrun
global invalTss
global segmentNotPresent
global stackException
global generalProtection
global pageFault
global coprError

; 外部中断
global hwint00
global hwint01
global hwint02
global hwint03
global hwint04
global hwint05
global hwint06
global hwint07
global hwint08
global hwint09
global hwint10
global hwint11
global hwint12
global hwint13
global hwint14
global hwint15

global sysCall

_start:
	mov esp, StackTop   ; 重新设置栈

	; 移动GDT
	sgdt [gdtPtr]       ; 将GdtPtr加载到内存中
	call kinit          ; 调用C函数重新设置GDT
	lgdt [gdtPtr]       ; 加载新的GDT

	lidt [idtPtr]       ; 加载IDT，中断向量表

	jmp	SELECTOR_KERNEL_CS:csinit

csinit:
	; 加载tss
	xor eax, eax
	mov ax, SELECTOR_TSS
	ltr ax

	jmp kmain

; 异常处理
; 这些地址都被填入中断向量表中，所以异常处理最后都会跳到exception，然后到C函数
; 异常发生时，会自动将 eflag、cs、eip或错误码压栈
; 如果没有错误码，我们自己压入0xFFFFFFFF，然后压入向量号，再调用异常处理函数
divideError:							; 除法错误
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
singleStepException:					; 调试异常
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
nmi:									; 不可屏蔽中断
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
breakpointException:					; 调试断点
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
overflow:								; 溢出
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
boundsCheck:							; 越界
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
invalOpcode:							; 未定义操作码
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
coprNotAvailable:						; 设备不可用
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
doubleFault:							; 双重错误
	push	8		; vector_no	= 8
	jmp	exception
coprSegOverrun:							; 协处理器段越界
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
invalTss:								; 无效TSS
	push	10		; vector_no	= A
	jmp	exception
segmentNotPresent:						; 段不存在
	push	11		; vector_no	= B
	jmp	exception
stackException:							; 堆栈段错误
	push	12		; vector_no	= C
	jmp	exception
generalProtection:						; 常规保护错误
	push	13		; vector_no	= D
	jmp	exception
pageFault:								; 页错误
	push	14		; vector_no	= E
	jmp	exception
coprError:								; 浮点错
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:								; 异常处理函数
	call	exceptionHandler
	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
	hlt

; 中断处理
%macro	hwint_master	1
	; 保存线程
	call	save

	; 关指定中断
	in	al, INT_M_CTLMASK
	or	al, (1 << %1)
	out	INT_M_CTLMASK, al
	mov	al, EOI
	out	INT_M_CTL, al

	; 打开CPU的中断响应
	sti

	; 调用中断处理函数
	push	%1
	call	[irqTable + 4 * %1]
	pop	ecx

	; 关闭CPU的中断响应
	cli

	; 开启指定的中断
	in	al, INT_M_CTLMASK
	and	al, ~(1 << %1)
	out	INT_M_CTLMASK, al
	ret	; 返回到save中压入的函数
%endmacro

ALIGN   16
hwint00:                ; Interrupt routine for irq 0 (the clock).
	hwint_master	0

ALIGN   16
hwint01:                ; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN   16
hwint02:                ; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN   16
hwint03:                ; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN   16
hwint04:                ; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN   16
hwint05:                ; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN   16
hwint06:                ; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN   16
hwint07:                ; Interrupt routine for irq 7 (printer)
	hwint_master	7

%macro  hwint_slave     1
    push    %1
    call    doIrq
    add     esp, 4
    hlt
%endmacro

ALIGN   16
hwint08:                ; Interrupt routine for irq 8 (realtime clock).
    hwint_slave     8

ALIGN   16
hwint09:                ; Interrupt routine for irq 9 (irq 2 redirected)
    hwint_slave     9

ALIGN   16
hwint10:                ; Interrupt routine for irq 10
    hwint_slave     10

ALIGN   16
hwint11:                ; Interrupt routine for irq 11
    hwint_slave     11

ALIGN   16
hwint12:                ; Interrupt routine for irq 12
    hwint_slave     12

ALIGN   16
hwint13:                ; Interrupt routine for irq 13 (FPU exception)
    hwint_slave     13

ALIGN   16
hwint14:                ; Interrupt routine for irq 14 (AT winchester)
    hwint_slave     14

ALIGN   16
hwint15:                ; Interrupt routine for irq 15
    hwint_slave     15

save:
	; 保存寄存器
	pushad
	push ds
	push es
	push fs
	push gs

	; 切换到内核的段
	; 在从低优先级到高优先级跳转的时候，硬件自动将ss和esp设置成TSS中设置的ss和esp
	; 并将原来的ss、esp、eflag、cs、eip压入栈中
	mov dx, ss
	mov ds, dx
	mov es, dx

	; 将当前栈顶保存下来
	mov esi, esp

	; 判断是否重入
	inc dword [kreenter]
	cmp dword [kreenter], 0
	jne .1

	; 使用内核栈
	mov esp, StackTop

	push restart
	jmp .2

.1:
	push reenter	; 中断重入

.2:
	jmp [esi + RETADR - P_STACKBASE] ; 函数调用返回




restart:
	mov esp, [procReady]				; 指向进程表项
	;lldt [esp + P_LDT_SEL]				; 加载LDT
	lea eax, [esp + P_STACKTOP]			; 移动到栈帧顶部
	mov dword [tss + TSS3_S_SP0], eax		; 将当前进程表项栈顶保存到tss中

reenter:
	dec dword [kreenter]

	; 从栈中弹出各个寄存器的值
	pop gs
	pop fs
	pop es
	pop ds
	popad

	add esp, 4

	; 返回进程执行
	iretd

sysCall:
	call save ; 保存现场
	push dword [procReady]
	sti		  ; 开中断
	push ecx
	push ebx
	call [sysCallTable + eax * 4]	; 调用相应的系统调用处理
	add esp, 4*3
	mov [esi + EAXREG - P_STACKBASE], eax	; 将系统调用返回值保存起来
	cli
	ret

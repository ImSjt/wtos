
NR_GET_TICKS equ 0
INT_VECTOR_SYS_CALL equ 0x90

global getTicks

bits 32
[section .text]

getTicks:
	mov eax, NR_GET_TICKS		; 通过寄存器来传递系统调用号
	int INT_VECTOR_SYS_CALL		; 触发系统调用
	ret


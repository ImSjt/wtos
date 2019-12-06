
NR_GET_TICKS equ 0
NR_WRITE	 equ 1

INT_VECTOR_SYS_CALL equ 0x90

global getTicks
global write

bits 32
[section .text]

getTicks:
	mov eax, NR_GET_TICKS		; 通过寄存器来传递系统调用号
	int INT_VECTOR_SYS_CALL		; 触发系统调用
	ret

write:
	mov eax, NR_WRITE
	mov ebx, [esp+4]
	mov ecx, [esp+8]
	int INT_VECTOR_SYS_CALL
	ret



;NR_TICKS	equ 0
NR_PRINTX	equ 0
NR_SENDREC	equ 1

INT_VECTOR_SYS_CALL equ 0x90

global printx
global sendrec

bits 32
[section .text]

; 支持传递三个参数，从右到左分别为 edx、ecx、ebx

printx:
	mov eax, NR_PRINTX
	mov edx, [esp+4]
	int INT_VECTOR_SYS_CALL
	ret

sendrec:
	mov eax, NR_SENDREC
	mov ebx, [esp+4]		; function
	mov ecx, [esp+8]		; obj
	mov edx, [esp+12]		; msg
	int INT_VECTOR_SYS_CALL
	ret
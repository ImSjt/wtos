[SECTION .text]

; 导出函数
global outByte
global inByte

; ------------------------------------------------------------------------
;                  void outByte(u16 port, u8 value);
; ------------------------------------------------------------------------
outByte:
	mov	edx, [esp + 4]		; port
	mov	al, [esp + 4 + 4]	; value
	out	dx, al
	nop	; 一点延迟
	nop
	ret

; ------------------------------------------------------------------------
;                  u8 inByte(u16 port);
; ------------------------------------------------------------------------
inByte:
	mov	edx, [esp + 4]		; port
	xor	eax, eax
	in	al, dx
	nop	; 一点延迟
	nop
	ret
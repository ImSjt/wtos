%include "const.inc"

[SECTION .text]

; 导出函数
global outByte
global inByte
global disableIrq
global enableIrq
global enableInt
global disableInt
global portRead
global portWrite

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

; ------------------------------------------------------------------------
;                  void disableIrq(int irq);
; ------------------------------------------------------------------------
disableIrq:
        mov     ecx, [esp + 4]          ; irq
        pushf
        cli
        mov     ah, 1
        rol     ah, cl                  ; ah = (1 << (irq % 8))
        cmp     cl, 8
        jae     disable8               ; disable irq >= 8 at the slave 8259
disable0:
        in      al, INT_M_CTLMASK
        test    al, ah
        jnz     disAlready             ; already disabled?
        or      al, ah
        out     INT_M_CTLMASK, al       ; set bit at master 8259
        popf
        mov     eax, 1                  ; disabled by this function
        ret
disable8:
        in      al, INT_S_CTLMASK
        test    al, ah
        jnz     disAlready             ; already disabled?
        or      al, ah
        out     INT_S_CTLMASK, al       ; set bit at slave 8259
        popf
        mov     eax, 1                  ; disabled by this function
        ret
disAlready:
        popf
        xor     eax, eax                ; already disabled
        ret

; ------------------------------------------------------------------------
;                  void enableIrq(int irq);
; ------------------------------------------------------------------------
enableIrq:
        mov     ecx, [esp + 4]          ; irq
        pushf
        cli
        mov     ah, ~1
        rol     ah, cl                  ; ah = ~(1 << (irq % 8))
        cmp     cl, 8
        jae     enable8                ; enable irq >= 8 at the slave 8259
enable0:
        in      al, INT_M_CTLMASK
        and     al, ah
        out     INT_M_CTLMASK, al       ; clear bit at master 8259
        popf
        ret
enable8:
        in      al, INT_S_CTLMASK
        and     al, ah
        out     INT_S_CTLMASK, al       ; clear bit at slave 8259
        popf
        ret

; ------------------------------------------------------------------------
;                  void enableInt();
; ------------------------------------------------------------------------
enableInt:
	sti
	ret

; ------------------------------------------------------------------------
;                  void disableInt();
; ------------------------------------------------------------------------
disableInt:
	cli
	ret

; ------------------------------------------------------------------------
;                  void portRead(u16 port, void* buf, int n);
; ------------------------------------------------------------------------
portRead:
	mov	edx, [esp + 4]		; port
	mov	edi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1
	cld
	rep	insw
	ret

; ------------------------------------------------------------------------
;                  void portWead(u16 port, void* buf, int n);
; ------------------------------------------------------------------------
portWrite:
	mov	edx, [esp + 4]		; port
	mov	esi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1
	cld
	rep	outsw
	ret
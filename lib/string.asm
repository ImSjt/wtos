extern dispPos

[SECTION .text]

; 导出函数
global memcpy
global memset
global dispStr
global dispColorStr
global strcpy
global strlen

; ------------------------------------------------------------------------
; void* memcpy(void* dest, void* src, int size);
; ------------------------------------------------------------------------
memcpy:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi			; ┃
					; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回

; ------------------------------------------------------------------------
; void memset(void* dst, char ch, int size);
; ------------------------------------------------------------------------
memset:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	edx, [ebp + 12]	; Char to be putted
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	byte [edi], dl		; ┓
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回

; ------------------------------------------------------------------------
;                  void dispStr(char* info);
; ------------------------------------------------------------------------
dispStr:
	push	ebp
	mov	ebp, esp

	mov	esi, [ebp + 8]	; info
	mov	edi, [dispPos]
	mov	ah, 0Fh
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[dispPos], edi

	pop	ebp
	ret

; ------------------------------------------------------------------------
;                  void dispColorStr(char* info, int color);
; ------------------------------------------------------------------------
dispColorStr:
	push	ebp
	mov	ebp, esp

	mov	esi, [ebp + 8]	; info
	mov	edi, [dispPos]
	mov	ah, [ebp + 12]	; color
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[dispPos], edi

	pop	ebp
	ret

; ------------------------------------------------------------------------
; char* strcpy(char* dst, char* src);
; ------------------------------------------------------------------------
strcpy:
	push    ebp
	mov     ebp, esp

	mov     esi, [ebp + 12] ; Source
	mov     edi, [ebp + 8]  ; Destination

.1:
	mov     al, [esi]               ; ┓
	inc     esi                     ; ┃
					; ┣ 逐字节移动
	mov     byte [edi], al          ; ┃
	inc     edi                     ; ┛

	cmp     al, 0           ; 是否遇到 '\0'
	jnz     .1              ; 没遇到就继续循环，遇到就结束

	mov     eax, [ebp + 8]  ; 返回值

	pop     ebp
	ret                     ; 函数结束，返回

; ------------------------------------------------------------------------
; int strlen(char* str);
; ------------------------------------------------------------------------
strlen:
        push    ebp
        mov     ebp, esp

        mov     eax, 0                  ; 字符串长度开始是 0
        mov     esi, [ebp + 8]          ; esi 指向首地址

.1:
        cmp     byte [esi], 0           ; 看 esi 指向的字符是否是 '\0'
        jz      .2                      ; 如果是 '\0'，程序结束
        inc     esi                     ; 如果不是 '\0'，esi 指向下一个字符
        inc     eax                     ;         并且，eax 自加一
        jmp     .1                      ; 如此循环

.2:
        pop     ebp
        ret                             ; 函数结束，返回
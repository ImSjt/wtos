org 07c00h ; 链接地址为07c00h

BaseOfStack		equ	07c00h

%include "load.inc"

    jmp short LABEL_START
    nop

; 包含FAT12文件系统格式的头部
%include "fat12hdr.inc"

LABEL_START:
    ; 初始化段寄存器
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov	sp, BaseOfStack 

    ; 清屏
    call ClearScreen
    
    ; 在第0行显示字符串
    mov dh, 0
    call DispStr

    ; 软驱复位
    xor ah, ah
    xor dl, dl
    int 13h

; 开始查找load.bin文件，并将其加载到内存中运行
    mov word [wSectorNo], SectorNoOfRootDirectory ; 根目录起始扇区号
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
    cmp word [wRootDirSizeForLoop], 0   ; 判断根目录区域是否查找完
    jz LABEL_NO_LOADERBIN               ; 没有loader.bin
    dec word [wRootDirSizeForLoop]

    ; 加载的内存地址
    mov ax, BaseOfLoader
    mov es, ax              ; 段地址
    mov	bx, OffsetOfLoader  ; 段内偏移

    ; 读取软盘
    mov	ax, [wSectorNo]     ; 要读取的扇区
    mov	cl, 1               ; 读取的扇区数
    call ReadSector         ; 通过BIOS中断来读取软盘

    mov si, LoaderFileName  ; ds:si <- 要查找的文件名
    mov di, OffsetOfLoader  ; es:di -> 从软盘中读取的目录项
    cld

    mov	dx, 10h             ; 一个目录项32字节，一个扇区512字节，所以一个扇区最大16个目录项

; 搜索读取到的扇区
LABEL_SEARCH_FOR_LOADERBIN:
    cmp dx, 0
    jz LABEL_SEARCH_NEXT_SECTOR     ; 搜索下一个扇区
    dec dx                          ; 当前扇区目录项数减1
    mov cx, 11                      ; 字符串长度

LABEL_CMP_FILENAME:
    cmp cx, 0
    jz LABEL_FILE_FOUND           ; 找到文件了
    dec	cx                        ; 剩余比较的字符数
    lodsb				          ; ds:si -> al
    cmp	al, byte [es:di]          ; 比较字符
    jz	LABEL_CMP_NEXT_C          ; 如果字符一样，则比较下一个字符
    jmp LABEL_CMP_NEXT_DENTRY     ; 如果字符不一样，则比较下一个目录项

LABEL_CMP_NEXT_C:
    inc	di                        ; 比较下一个字符
    jmp	LABEL_CMP_FILENAME        ; 继续比较

LABEL_CMP_NEXT_DENTRY:
    ; 指向下一条目录项
    and	di, 0FFE0h
    add	di, 20h
    
    mov	si, LoaderFileName          ; 重新指向查找的文件名起始处
    jmp LABEL_SEARCH_FOR_LOADERBIN  ; 比较下一个目录项

LABEL_SEARCH_NEXT_SECTOR:
    add word [wSectorNo], 1             ; 下一个扇区
    jmp	LABEL_SEARCH_IN_ROOT_DIR_BEGIN  ; 重新开始搜索

LABEL_FILE_FOUND:
    mov ax, RootDirSectors
    and di, 0FFE0h          ; 指向当前目录项
    add di, 01Ah            ; 指向该文件数据区起始簇号
    mov cx, word[es:edi]    ; 得到该文件数据区起始簇号
    push cx                 ; 暂且将其保存下来
    add cx, ax
    add cx, DeltaSectorNo   ; 得到簇对应的扇区号
    mov ax, BaseOfLoader    ; 加载的段起始地址
    mov es, ax
    mov bx, OffsetOfLoader  ; 加载的段内偏移值
    mov ax, cx              ; 要读取的扇区号

LABEL_GOON_LOADING_FILE:
    ; 在 Booting后面打印一个点
    call DispPoint

    mov cl, 1
    call ReadSector         ; 读取软盘扇区，也就是loader.bin在数据区中的首扇区
    call DispPoint

    pop ax
    call GetFATEntry        ; 得到该扇区在FAT中对应的项指向的下一个簇号
    cmp ax, 0FFFh           ; 如果等于0FFFh，说明已经是最后一项了
    
    ; 开始加载loader.bin
    jz LABEL_FILE_LOADED    

    ; 否则，读取文件的下一个扇区
    push ax

    ; 计算簇号对应的扇区编号
    mov dx, RootDirSectors
    add ax, dx
    add ax, DeltaSectorNo

    add bx, [BPB_BytsPerSec]    ; 加载目的地址往后移动一个扇区的大小
    jmp LABEL_GOON_LOADING_FILE

LABEL_FILE_LOADED:
    ; 准备就绪，开始跳转
    mov dh, 1
    call DispStr

    jmp BaseOfLoader:OffsetOfLoader     ; 跳转到loader.bin运行

LABEL_NO_LOADERBIN:
    mov dh, 2
    call DispStr
    jmp	$

;变量
wRootDirSizeForLoop	dw	RootDirSectors; Root Directory 占用的扇区数, 在循环中会递减至零.
wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数

LoaderFileName		db	"LOADER  BIN", 0

MessageLength		equ	9
BootMessage:		db	"Booting  "; 9字节, 不够则用空格补齐. 序号 0
Message1		db	"Ready.   "; 9字节, 不够则用空格补齐. 序号 1
Message2		db	"No LOADER"; 9字节, 不够则用空格补齐. 序号 2

; 显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStr:
	mov	ax, MessageLength
	mul	dh
	add	ax, BootMessage
	mov	bp, ax			    ; ┓
	mov	ax, ds			    ; ┣ ES:BP = 串地址
	mov	es, ax			    ; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 01301h		    ; AH = 13,  AL = 01h
	mov	bx, 0007h		    ; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov	dl, 0
	int	10h			        ; int 10h
	ret

DispPoint:
    push ax
    push bx
    mov ah, 0Eh
    mov al, '.'
    mov bl, 0Fh
    int 10h
    pop bx
    pop ax
    ret

; 清屏操作
ClearScreen:
    mov	ax, 0600h		; AH = 6,  AL = 0h
    mov	bx, 0700h		; 黑底白字(BL = 07h)
    mov	cx, 0			; 左上角: (0, 0)
    mov	dx, 0184fh		; 右下角: (80, 50)
    int 10h
    ret

; 从第 ax 个 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
ReadSector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                          ┌ 柱面号 = y >> 1，每个柱面两个磁头
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数      │
	;                   └ 余 z => 起始扇区号 = z + 1
	push	bp
	mov	bp, sp
	sub	esp, 2			; 辟出两个字节的堆栈区域保存要读的扇区数: byte [bp-2]

	mov	byte [bp-2], cl ; 保存要读的扇区数
	push	bx			; 保存 bx，读入的地址
	mov	bl, [BPB_SecPerTrk]	; bl: 除数，每磁道扇区数
	div	bl			    ; y 在 al 中, z 在 ah 中，求ax / bl
	inc	ah			    ; z ++，起始扇区号
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y，，磁头号
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)，柱面号
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			    ; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
	mov	dl, [BS_DrvNum]	; 驱动器号 (0 表示 A 盘)
.GoOnReading:
	mov	ah, 2			; 读
	mov	al, byte [bp-2]	; 读 al 个扇区
	int	13h
	jc	.GoOnReading	; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	add	esp, 2
	pop	bp

	ret

; 作用:
;	找到序号为 ax 的 Sector 在 FAT 中的条目, 结果放在 ax 中
;	需要注意的是, 中间需要读 FAT 的扇区到 es:bx 处, 所以函数一开始保存了 es 和 bx
GetFATEntry:
	push	es
	push	bx
	push	ax
	mov	ax, BaseOfLoader; `.
	sub	ax, 0100h	;  | 在 BaseOfLoader 后面留出 4K 空间用于存放 FAT
	mov	es, ax		; /
	pop	ax
	mov	byte [bOdd], 0
	mov	bx, 3
	mul	bx			; dx:ax = ax * 3
	mov	bx, 2
	div	bx			; dx:ax / 2  ==>  ax <- 商, dx <- 余数
	cmp	dx, 0
	jz	LABEL_EVEN
	mov	byte [bOdd], 1
LABEL_EVEN:;偶数
	; 现在 ax 中是 FATEntry 在 FAT 中的偏移量,下面来
	; 计算 FATEntry 在哪个扇区中(FAT占用不止一个扇区)
	xor	dx, dx			
	mov	bx, [BPB_BytsPerSec]
	div	bx ; dx:ax / BPB_BytsPerSec
		   ;  ax <- 商 (FATEntry 所在的扇区相对于 FAT 的扇区号)
		   ;  dx <- 余数 (FATEntry 在扇区内的偏移)。
	push	dx
	mov	bx, 0 ; bx <- 0 于是, es:bx = (BaseOfLoader - 100):00
	add	ax, SectorNoOfFAT1 ; 此句之后的 ax 就是 FATEntry 所在的扇区号
	mov	cl, 2
	call	ReadSector ; 读取 FATEntry 所在的扇区, 一次读两个, 避免在边界
			   ; 发生错误, 因为一个 FATEntry 可能跨越两个扇区
	pop	dx
	add	bx, dx
	mov	ax, [es:bx]
	cmp	byte [bOdd], 1
	jnz	LABEL_EVEN_2
	shr	ax, 4
LABEL_EVEN_2:
	and	ax, 0FFFh

LABEL_GET_FAT_ENRY_OK:

	pop	bx
	pop	es
	ret

times 	510-($-$$)	db	0
dw 	0xaa55 ; 最后填充512个字节，0xaa55结尾，以使BIOS识别这是一个启动盘
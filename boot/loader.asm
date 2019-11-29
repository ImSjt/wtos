org 0100h	; 段内预留0100h大小作为临时栈使用

    jmp LABEL_START

%include "load.inc"
%include "fat12hdr.inc"
%include "pm.inc"
 
; GDT
; 段基址，段界限，段属性
; 段的起始地址都是从0开始，即逻辑地址等于线性地址
LABEL_GDT: Descriptor 0, 0, 0
LABEL_DESC_KERNEL_CS: Descriptor 0, 0FFFFFh, DA_CR|DA_32|DA_LIMIT_4K       	  ; 0-4G，内核代码段
LABEL_DESC_KERNEL_DS: Descriptor 0, 0FFFFFh,  DA_DRW|DA_32|DA_LIMIT_4K    	  ; 0-4G，内核数据段
LABEL_DESC_TASK_CS: Descriptor 0, 0FFFFFh, DA_CR|DA_32|DA_LIMIT_4K|DA_DPL1 	  ; 0-4G，内核代码段
LABEL_DESC_TASK_DS: Descriptor 0, 0FFFFFh,  DA_DRW|DA_32|DA_LIMIT_4K|DA_DPL1 ; 0-4G，内核数据段
LABEL_DESC_USER_CS: Descriptor 0, 0FFFFFh, DA_CR|DA_32|DA_LIMIT_4K|DA_DPL3    ; 0-4G，用户代码段
LABEL_DESC_USER_DS: Descriptor 0, 0FFFFFh,  DA_DRW|DA_32|DA_LIMIT_4K|DA_DPL3  ; 0-4G，用户数据段
LABEL_DESC_VIDEO: Descriptor 0B8000h, 0FFFFFh,  DA_DRW|DA_DPL3         		  ; 显存首地址，显存段

GdtLen equ $-LABEL_GDT

; GDTPTR寄存器加载值
GdtPtr dw GdtLen - 1    ; 段界限
dd BaseOfLoaderPhyAddr + LABEL_GDT  ; 段物理地址

; GDT选择子
SelectorKernelCS equ LABEL_DESC_KERNEL_CS - LABEL_GDT				; 内核代码段选择子
SelectorKernelDS equ LABEL_DESC_KERNEL_DS - LABEL_GDT				; 内核数据段选择子
SelectorTaskCS equ LABEL_DESC_TASK_CS - LABEL_GDT + SA_RPL1			; 用户代码段选择子
SelectorTaskDS equ LABEL_DESC_TASK_DS - LABEL_GDT + SA_RPL1			; 用户数据段选择子
SelectorUserCS equ LABEL_DESC_USER_CS - LABEL_GDT + SA_RPL3			; 用户代码段选择子
SelectorUserDS equ LABEL_DESC_USER_DS - LABEL_GDT + SA_RPL3			; 用户数据段选择子
SelectorVideo equ LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3			; 显存段选择子

BaseOfStack equ 0100h		; 栈基址
PageDirBase	equ	100000h	; 页目录开始地址: 1M
PageTblBase	equ	101000h	; 页表开始地址:   1M + 4K

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BaseOfStack

    mov dh, 0
    call DispStrRealMode	; 显示 "Loading."

    ; 获取物理内存空间范围
    mov ebx, 0
    mov di, _MemChkBuf ; es:di 指向一个地址范围描述符结构(ARDS)
.MemChkLoop:
    mov eax, 0E820h
    mov	ecx, 20			; ecx = 地址范围描述符结构的大小
    mov	edx, 0534D4150h		; edx = 'SMAP'
    int	15h			; int 15h
    jc	.MemChkFail
    add di, 20
    inc dword [_dwMCRNumber]  ; dwMCRNumber = ARDS 的个数
    cmp ebx, 0
    jne .MemChkLoop             
    jmp .MemChkOK              ; 得到所有的地址范围描述符

.MemChkFail:
    mov dword [_dwMCRNumber], 0
.MemChkOK:

    ; 首先在软盘A中查找kernel.bin文件
    mov word [wSectorNo], SectorNoOfRootDirectory

    ; 软驱复位
    xor ah, ah
    xor dl, dl
    int 13h

LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
    cmp word [wRootDirSizeForLoop], 0
    jz LABEL_NO_KERNEL                  ; 找不到内核
    dec word [wRootDirSizeForLoop]
    
    ; 读取根目录区扇区
    mov ax, BaseOfKernelFile
    mov es, ax
    mov bx, OffsetOfKernelFile
    mov ax, [wSectorNo]
    mov cl, 1
    call ReadSector

    mov si, KernelFileName
    mov di, OffsetOfKernelFile
    cld
    mov dx, 10h                 ; 一个扇区的目录项数

LABEL_SEARCH_FOR_KERNELBIN:
    cmp dx, 0
    jz LABEL_SEARCH_NEXT_SECTOR ; 查找下一个扇区
    dec dx

    mov cx, 11 ; 文件名长度

LABEL_CMP_FILENAME:
    cmp cx, 0
    jz LABEL_FILE_FOUND     ; 找到文件了
    dec cx
    lodsb
    cmp al, byte [es:di]
    jz LABEL_CMP_NEXT_C     ; 如果当前字符相等，那么比较下一个字符
    jmp LABEL_CMP_NEXT_DENTRY   ; 否则，比较下一个条目

LABEL_CMP_NEXT_C:
    inc di
    jmp LABEL_CMP_FILENAME

LABEL_CMP_NEXT_DENTRY:
    ; 移动到下一个条目起始处
    and di, 0FFE0h
    add di, 20h
    
    mov si, KernelFileName
    jmp LABEL_SEARCH_FOR_KERNELBIN

LABEL_SEARCH_NEXT_SECTOR:
    add word [wSectorNo], 1
    jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN

LABEL_NO_KERNEL:
    mov dh, 2
    call DispStrRealMode
    jmp $

LABEL_FILE_FOUND:
	mov ax, RootDirSectors
	and di, 0FFF0h          ; 单前目录项条目

	push eax
	mov eax, [es:di + 01Ch] ; 得到文件的大小
	mov dword [dwKernelSize], eax
	pop eax

	add di, 01Ah            ; 文件的首簇号
	mov cx, word [es:di]
	push cx

	; 计算对应的扇区号
	add cx, ax
	add cx, DeltaSectorNo

	; 为读取软盘做准备
	mov ax, BaseOfKernelFile
	mov es, ax
	mov bx, OffsetOfKernelFile
	mov ax, cx

LABEL_GOON_LOADING_FILE:
	; 每加载一个扇区就打印一个点
	push ax
	push bx
	mov ah, 0Fh
	mov al, '.'
	mov bl, 0Fh
	int 10h
	pop bx
	pop ax

	; 读取一个扇区
	mov cl, 1
	call ReadSector

	; 读取该扇区在FAT区中对应的内容
	pop ax
	call GetFATEntry
	cmp ax, 0FFFh
	jz LABEL_FILE_LOADER    ; 如果文件读取完了，就加载文件

	; 将簇号转换为扇区号
	push ax
	mov dx, RootDirSectors
	add ax, dx
	add ax, DeltaSectorNo
	add bx, [BPB_BytsPerSec]
	jmp LABEL_GOON_LOADING_FILE

LABEL_FILE_LOADER:

	call KillMotor ; 关闭软驱马达

	mov dh, 1
	call DispStrRealMode

	lgdt [GdtPtr]   ; 加载GDTR

	cli             ; 关中断

	; 打开地址线A20
	in al, 92h
	or al, 00000010b
	out 92h, al

	; 打开保护模式
	mov eax, cr0
	or eax, 1
	mov cr0, eax

	; 真正进入保护模式
	jmp dword SelectorKernelCS:(BaseOfLoaderPhyAddr+LABEL_PM_START)

wRootDirSizeForLoop	dw	RootDirSectors	; Root Directory 占用的扇区数
wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数
dwKernelSize		dd	0		; KERNEL.BIN 文件大小

KernelFileName		db	"KERNEL  BIN", 0	; KERNEL.BIN 之文件名

MessageLength		equ	9
LoadMessage:		db	"Loading  "
Message1		db	"Ready.   "
Message2		db	"No KERNEL"

; 显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStrRealMode:
	mov	ax, MessageLength
	mul	dh
	add	ax, LoadMessage
	mov	bp, ax			; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 01301h		; AH = 13,  AL = 01h
	mov	bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov	dl, 0
	add	dh, 3			; 从第 3 行往下显示
	int	10h			; int 10h
	ret

; 从序号(Directory Entry 中的 Sector 号)为 ax 的的 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
ReadSector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数     │
	;                   └ 余 z => 起始扇区号 = z + 1
	push	bp
	mov	bp, sp
	sub	esp, 2			; 辟出两个字节的堆栈区域保存要读的扇区数: byte [bp-2]

	mov	byte [bp-2], cl
	push	bx			; 保存 bx
	mov	bl, [BPB_SecPerTrk]	; bl: 除数
	div	bl			; y 在 al 中, z 在 ah 中
	inc	ah			; z ++
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
	mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
.GoOnReading:
	mov	ah, 2			; 读
	mov	al, byte [bp-2]		; 读 al 个扇区
	int	13h
	jc	.GoOnReading		; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	add	esp, 2
	pop	bp

	ret

GetFATEntry:
	push	es
	push	bx
	push	ax
	mov	ax, BaseOfKernelFile	; ┓
	sub	ax, 0100h		; ┣ 在 BaseOfKernelFile 后面留出 4K 空间用于存放 FAT
	mov	es, ax			; ┛
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
	xor	dx, dx			; 现在 ax 中是 FATEntry 在 FAT 中的偏移量. 下面来计算 FATEntry 在哪个扇区中(FAT占用不止一个扇区)
	mov	bx, [BPB_BytsPerSec]
	div	bx			; dx:ax / BPB_BytsPerSec  ==>	ax <- 商   (FATEntry 所在的扇区相对于 FAT 来说的扇区号)
					;				dx <- 余数 (FATEntry 在扇区内的偏移)。
	push	dx
	mov	bx, 0			; bx <- 0	于是, es:bx = (BaseOfKernelFile - 100):00 = (BaseOfKernelFile - 100) * 10h
	add	ax, SectorNoOfFAT1	; 此句执行之后的 ax 就是 FATEntry 所在的扇区号
	mov	cl, 2
	call	ReadSector		; 读取 FATEntry 所在的扇区, 一次读两个, 避免在边界发生错误, 因为一个 FATEntry 可能跨越两个扇区
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

; 关闭软驱马达
KillMotor:
	push	dx
	mov	dx, 03F2h
	mov	al, 0
	out	dx, al
	pop	dx
	ret

; 32位的代码，进入保护模式后运行
[SECTION .s32]
ALIGN 32
[BITS 32]
LABEL_PM_START:
	mov ax, SelectorVideo
	mov gs, ax
	;mov ah, 0Fh
	;mov al, 'p'
	;mov [gs:((80*0 + 39)*2)], ax

	; 初始化段寄存器
	mov ax, SelectorKernelDS
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ss, ax

	; 设置栈顶
	mov esp, TopOfStack

	push szMemChkTitle
	call DispStr
	add esp, 4

	call DispMemInfo
	call SetupPaging

	; 将内核elf文件加载到内核，并移动到链接地址
	call InitKernel

	jmp SelectorKernelCS:KernelEntryPointPhyAddr ; 跳转执行内核

%include "lib.inc"  ; 包含打印的函数

; 显示内存信息
DispMemInfo:
	push	esi
	push	edi
	push	ecx

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber];for(int i=0;i<[MCRNumber];i++)//每次得到一个ARDS
.loop:				  ;{
	mov	edx, 5		  ;  for(int j=0;j<5;j++)//每次得到一个ARDS中的成员
	mov	edi, ARDStruct	  ;  {//依次显示:BaseAddrLow,BaseAddrHigh,LengthLow
.1:				  ;               LengthHigh,Type
	push	dword [esi]	  ;
	call	DispInt		  ;    DispInt(MemChkBuf[j*4]); // 显示一个成员
	pop	eax		  ;
	stosd			  ;    ARDStruct[j*4] = MemChkBuf[j*4];
	add	esi, 4		  ;
	dec	edx		  ;
	cmp	edx, 0		  ;
	jnz	.1		  ;  }
	call	DispReturn	  ;  printf("\n");
	cmp	dword [dwType], 1 ;  if(Type == AddressRangeMemory)
	jne	.2		  ;  {
	mov	eax, [dwBaseAddrLow];
	add	eax, [dwLengthLow];
	cmp	eax, [dwMemSize]  ;    if(BaseAddrLow + LengthLow > MemSize)
	jb	.2		  ;
	mov	[dwMemSize], eax  ;    MemSize = BaseAddrLow + LengthLow;
.2:				  ;  }
	loop	.loop		  ;}
				  ;
	call	DispReturn	  ;printf("\n");
	push	szRAMSize	  ;
	call	DispStr		  ;printf("RAM size:");
	add	esp, 4		  ;
				  ;
	push	dword [dwMemSize] ;
	call	DispInt		  ;DispInt(MemSize);
	add	esp, 4		  ;

	pop	ecx
	pop	edi
	pop	esi
	ret

; 启动分页，这一块重新设置
SetupPaging:
    
    xor edx, edx
    mov eax, [dwMemSize]

    ; 一个页表有1024项，每项对应一个物理页4096个字节
    mov ebx, 400000h	; 400000h = 4M = 4096 * 1024
    div ebx             ; 计算需要多少个页表
    mov ecx, eax
    test edx, edx       ; 求余
    jz .no_remainder
    inc ecx

.no_remainder:
    push ecx        ; 暂存需要的页表个数

    mov ax, SelectorKernelDS
    mov es, ax
    mov edi, PageDirBase    ; 页目录项首地址
    xor eax, eax
    mov eax, PageTblBase | PG_P  | PG_USU | PG_RWW

; 初始化页目录项
.1:
    stosd
    add eax, 4096
    loop .1

    ; 初始化所有页表
    pop eax     ; 页表个数
    mov ebx, 1024       ;每个页表有1024个页表项
    mul ebx     ; 求得总的页表项，结果在eax中
    mov ecx, eax
    mov edi, PageTblBase    ; 页表基址
    xor eax, eax
    mov eax, PG_P  | PG_USU | PG_RWW

.2:
    stosd
    add eax, 4096   ;物理地址的下4K空间
    loop .2

    ; 开启分页
    mov eax, PageDirBase
    mov cr3, eax
    mov	eax, cr0
    or	eax, 80000000h
    mov	cr0, eax
    jmp	short .3

.3:
	nop

	ret

; 下面根据elf中的program header，将对应的段拷贝到对应的虚拟地址中
InitKernel:
    xor esi, esi
    mov cx, word [BaseOfKernelFilePhyAddr + 2Ch]  ; pELFHdr->e_phnum
    movzx ecx, cx                                 ; Program header的个数
    mov esi, [BaseOfKernelFilePhyAddr + 1Ch]      ; pELFHdr->e_phoff
    add esi, BaseOfKernelFilePhyAddr              ; 得到Program header在内存中的位置

.Begin:
    mov eax, [esi + 0]
    cmp eax, 0
    jz .NoAction
    push dword [esi + 010h]     ; 将段在文件中的长度压入栈中暂存
    mov eax, [esi + 04h]        ; 该段第一个字节在文件中的偏移值
    add eax, BaseOfKernelFilePhyAddr    ; 指向该段在内存中的位置，准备重定位
    push eax                    ; src
    push dword [esi+08h]        ; dst

    ; 到此，栈中压入了三个参数，段大小，段源地址，段目的地址
    call MemCpy     ; 调用拷贝函数
    add esp, 12     ; 弹出参数

.NoAction:
    add esi, 020h   ; 移动到下一条 Program header
    dec ecx
    jnz .Begin

    ret

[SECTION .data1]

ALIGN	32

LABEL_DATA:
; 实模式下使用这些符号
; 字符串
_szMemChkTitle:	db "BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:	db "RAM size:", 0
_szReturn:	db 0Ah, 0
;; 变量
_dwMCRNumber:	dd 0	; Memory Check Result
_dwDispPos:	dd (80 * 6 + 0) * 2	; 屏幕第 6 行, 第 0 列。
_dwMemSize:	dd 0
_ARDStruct:	; Address Range Descriptor Structure
  _dwBaseAddrLow:		dd	0
  _dwBaseAddrHigh:		dd	0
  _dwLengthLow:			dd	0
  _dwLengthHigh:		dd	0
  _dwType:			dd	0
_MemChkBuf:	times	256	db	0

; 保护模式下使用这些符号
szMemChkTitle		equ	BaseOfLoaderPhyAddr + _szMemChkTitle
szRAMSize		equ	BaseOfLoaderPhyAddr + _szRAMSize
szReturn		equ	BaseOfLoaderPhyAddr + _szReturn
dwDispPos		equ	BaseOfLoaderPhyAddr + _dwDispPos
dwMemSize		equ	BaseOfLoaderPhyAddr + _dwMemSize
dwMCRNumber		equ	BaseOfLoaderPhyAddr + _dwMCRNumber
ARDStruct		equ	BaseOfLoaderPhyAddr + _ARDStruct
dwBaseAddrLow	equ	BaseOfLoaderPhyAddr + _dwBaseAddrLow
dwBaseAddrHigh	equ	BaseOfLoaderPhyAddr + _dwBaseAddrHigh
dwLengthLow	equ	BaseOfLoaderPhyAddr + _dwLengthLow
dwLengthHigh	equ	BaseOfLoaderPhyAddr + _dwLengthHigh
dwType		equ	BaseOfLoaderPhyAddr + _dwType
MemChkBuf		equ	BaseOfLoaderPhyAddr + _MemChkBuf

; 分配一段栈空间
StackSpace: times 1024 db 0
TopOfStack equ BaseOfLoaderPhyAddr + $  ; 栈顶物理地址

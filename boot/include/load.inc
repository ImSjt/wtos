BaseOfLoader	    equ	 09000h	; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader	    equ	  0100h	; LOADER.BIN 被加载到的位置 ---- 偏移地址

BaseOfLoaderPhyAddr equ	BaseOfLoader * 10h ; LOADER.BIN 被加载到的位置 ---- 物理地址

BaseOfKernelFile    equ	 08000h	; KERNEL.BIN 被加载到的位置 ----  段地址
OffsetOfKernelFile  equ	     0h	; KERNEL.BIN 被加载到的位置 ---- 偏移地址

BaseOfKernelFilePhyAddr equ BaseOfKernelFile*10h ; 内核被加载的物理地址

KernelEntryPointPhyAddr equ 030400h             ; 内核的链接地址，即最后拷贝到虚拟内存中的地址
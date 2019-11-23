# 链接的目的虚拟地址，必须与引导程序中的KernelEntryPointPhyAddr一样
ENTRYPOINT	= 0x30400

ENTRYOFFSET	=   0x400

TOPDIR	= $(shell pwd)
export TOPDIR

ASM			= nasm
DASM		= ndisasm
CC			= gcc
CXX			= g++
LD			= ld
ASMBFLAGS	= -I $(TOPDIR)/boot/include/
ASMKFLAGS	= -I $(TOPDIR)/include/ -f elf
CFLAGS		= -m32 -I $(TOPDIR)/include/ -c -fno-builtin -fno-stack-protector
CXXFLAGS	= -m32 -I $(TOPDIR)/include/ -c -fno-builtin -fno-stack-protector
LDFLAGS		= -m elf_i386 -s
LDADDR		= -Ttext $(ENTRYPOINT)
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

export ASM DASM CC CXX LD ASMBFLAGS ASMKFLAGS CFLAGS CXXFLAGS LDFLAGS DASMFLAGS

TARGET = kernel.bin
DASMOUTPUT	= kernel.bin.asm

OBJ_LIB		= $(TOPDIR)/lib/build-in.o
OBJ_KERNEL	= $(TOPDIR)/kernel/build-in.o
OBJ			= $(OBJ_KERNEL) $(OBJ_LIB)

.PHONY : all boot kernel lib
all : boot kernel lib
	ld $(LDFLAGS) $(LDADDR) -o $(TARGET) $(OBJ)

boot :
	make -C $(TOPDIR)/boot/

kernel :
	make -C $(TOPDIR)/kernel/

lib :
	make -C $(TOPDIR)/lib/

disasm :
	$(DASM) $(DASMFLAGS) $(TARGET) > $(DASMOUTPUT)

image:
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

.PHONY : clean
clean :
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.bin")
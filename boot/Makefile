TARGET		= boot.bin loader.bin	# 目标文件

# .PHONY 表明这是一个标记
.PHONY : all clean

# 该Makfile的首个规则
all : $(TARGET)

clean :
	rm -f $(TARGET)

# boot.bin 的编译规则
boot.bin : boot.asm include/load.inc include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

# loader.bin 的编译规则
loader.bin : loader.asm include/load.inc include/fat12hdr.inc include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<
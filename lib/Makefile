OBJ = string.o kliba.o klib.o printf.o vsprintf.o assert.o open.o close.o read.o write.o unlink.o

all : build-in.o

build-in.o : $(OBJ)
	$(LD) $(LDFLAGS) -r -o $@ $^

%.o : %.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<
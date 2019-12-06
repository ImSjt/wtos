OBJ = string.o kliba.o klib.o printf.o vsprintf.o

all : build-in.o

build-in.o : $(OBJ)
	$(LD) $(LDFLAGS) -r -o $@ $^

%.o : %.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<
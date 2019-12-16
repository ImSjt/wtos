#include "type.h"
#include "stdio.h"
#include "tty.h"

void spin(char * funcName)
{
	printf("\nspinning in %s ...\n", funcName);
	while (1) {}
}

void assertionFailure(char *exp, char *file, char *baseFile, int line)
{
	printf("%c  assert(%s) failed: file: %s, base_file: %s, ln%d",
	       MAG_CH_ASSERT,
	       exp, file, baseFile, line);

	spin("assertionFailure()");

	/* 不可以到达这里 */
    __asm__ __volatile__("ud2");
}

void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printf("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}


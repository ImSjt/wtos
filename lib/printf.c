#include "type.h"
#include "proto.h"
#include "stdio.h"

int printf(const char* fmt, ...)
{
    int i;
    char buf[256];

    va_list arg = (va_list)((char*)(&fmt) + 4); /* 指向参数列表 */
    i = vsprintf(buf, fmt, arg);

    buf[i] = '\0';
    printx(buf);

    return i;
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list arg = (va_list)((char*)(&fmt) + 4);        /* 4 是参数 fmt 所占堆栈中的大小 */
	return vsprintf(buf, fmt, arg);
}

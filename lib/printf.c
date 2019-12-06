#include "type.h"
#include "proto.h"
#include "stdio.h"

int printf(const char* fmt, ...)
{
    int i;
    char buf[256];

    va_list arg = (va_list)((char*)(&fmt) + 4); /* 指向参数列表 */
    i = vsprintf(buf, fmt, arg);
    write(buf, i);

    return i;
}


#include "type.h"
#include "string.h"

int vsprintf(char* buf, const char* fmt, va_list args)
{
    char* p;
    char tmp[256];
    va_list nextArg = args;

    for(p = buf; *fmt; ++fmt)
    {
        if(*fmt != '%')
        {
            *p++ = *fmt;
            continue;
        }

        fmt++;

        switch(*fmt)
        {
            case 'x':
            {
                itoa(tmp, *((int*)nextArg));
                strcpy(p, tmp);
                nextArg += 4;
                p += strlen(tmp);
                break;
            }
            case 's':
            {
                break;
            }
            default:
            {
                break;
            }
        }
        
    }

    return p-buf;
}

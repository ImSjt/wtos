#include "type.h"
#include "string.h"

static char* i2a(int val, int base, char** ps)
{
	int m = val % base;
	int q = val / base;
	if (q)
    {
		i2a(q, base, ps);
	}
    
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

	return *ps;
}

int vsprintf(char* buf, const char* fmt, va_list args)
{
	char* p;

	va_list	nextArg = args;
	int	m;

	char innerBuf[1024];
	char cs;
	int	alignNr;

	for(p=buf; *fmt; fmt++)
    {
		if (*fmt != '%')
        {
			*p++ = *fmt;
			continue;
		}
		else
        {
			alignNr = 0;
		}

		fmt++;

		if(*fmt == '%')
        {
			*p++ = *fmt;
			continue;
		}
		else if(*fmt == '0')
        {
			cs = '0';
			fmt++;
		}
		else
        {
			cs = ' ';
		}
        
		while (((unsigned char)(*fmt) >= '0') && ((unsigned char)(*fmt) <= '9'))
        {
			alignNr *= 10;
			alignNr += *fmt - '0';
			fmt++;
		}

		char* q = innerBuf;
		memset(q, 0, sizeof(innerBuf));

		switch (*fmt)
        {
		case 'c':
			*q++ = *((char*)nextArg);
			nextArg += 4;
			break;
		case 'x':
			m = *((int*)nextArg);
			i2a(m, 16, &q);
			nextArg += 4;
			break;
		case 'd':
			m = *((int*)nextArg);
			if (m < 0) 
            {
				m = m * (-1);
				*q++ = '-';
			}
			i2a(m, 10, &q);
			nextArg += 4;
			break;
		case 's':
			strcpy(q, (*((char**)nextArg)));
			q += strlen(*((char**)nextArg));
			nextArg += 4;
			break;
		default:
			break;
		}

		int k;
		for(k = 0; k < ((alignNr > strlen(innerBuf)) ? (alignNr - strlen(innerBuf)) : 0); k++)
        {
			*p++ = cs;
		}
		q = innerBuf;
		while (*q)
        {
			*p++ = *q++;
		}
	}

	*p = 0;

	return (p - buf);
}


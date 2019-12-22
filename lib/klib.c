#include "type.h"
#include "string.h"

/* 将数字转成字符串 */
char* itoa(char* str, int num)
{
	char* p = str;
	char ch;
	int	i;
	int	flag = 0;

	*p++ = '0';
	*p++ = 'x';

	if(num == 0)
    {
		*p++ = '0';
	}
	else
    {
		for(i=28;i>=0;i-=4)
        {
			ch = (num >> i) & 0xF;
			if(flag || (ch > 0))
            {
				flag = 1;
				ch += '0';
				if(ch > '9')
                {
					ch += 7;
				}
				*p++ = ch;
			}
		}
	}

	*p = 0;

	return str;
}

void dispInt(int input)
{
	char output[16];
	itoa(output, input);
	dispStr(output);
}

void delay(int time)
{
	int i, j, k;
	for(k = 0; k < time; k++)
	{
		for(i = 0; i < 10; i++)
		{
			for(j = 0; j < 10000; j++)
			{ }
		}
	}
}

int memcmp(const void* s1, const void*s2, int n)
{
	if ((s1 == 0) || (s2 == 0))
    {
        /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = (const char *)s1;
	const char * p2 = (const char *)s2;
	int i;
	for (i = 0; i < n; i++,p1++,p2++)
    {
		if (*p1 != *p2)
        {
			return (*p1 - *p2);
		}
	}
	return 0;
}

int strcmp(const char* s1, const char* s2)
{
	if ((s1 == 0) || (s2 == 0))
    {
        /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = s1;
	const char * p2 = s2;

	for (; *p1 && *p2; p1++,p2++)
    {
		if (*p1 != *p2)
        {
			break;
		}
	}

	return (*p1 - *p2);
}


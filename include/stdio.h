#ifndef _STDIO_H_
#define _STDIO_H_
#include "type.h"

int printf(const char* fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char* buf, const char* fmt, va_list args);

#define ASSERT
#ifdef ASSERT
void assertionFailure(char *exp, char *file, char *base_file, int line);
#define assert(exp) if (exp) ; \
                    else assertionFailure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

void spin(char* funcName);
void panic(const char *fmt, ...);
int printk(const char* fmt, ...);

#define	Max(a,b)	((a) > (b) ? (a) : (b))
#define	Min(a,b)	((a) < (b) ? (a) : (b))

#endif /* _STDIO_H_ */

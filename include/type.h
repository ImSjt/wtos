#ifndef	_TYPE_H_
#define	_TYPE_H_

#ifdef __cplusplus
#define C "C"
#define CPP
#else
#define C
#endif

typedef unsigned long long  u64;
typedef	unsigned int		u32;
typedef	unsigned short		u16;
typedef	unsigned char		u8;

typedef void (*irqHandler)(int);

typedef void* systemCall;

typedef char* va_list;

#endif /* _TYPE_H_ */

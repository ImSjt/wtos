#ifndef _STRING_H_
#define _STRING_H_

void* memcpy(void* dest,const void* src, int size);
void memset(void* dst, char ch, int size);
int memcmp(const void* s1, const void*s2, int n);

void dispStr(const char* info);
void dispColorStr(const char* info, int color);
void dispInt(int input);

void delay(int time);

char* itoa(char* str, int num);
char* strcpy(char* dst, char* src);
int strlen(const char* str);
int strcmp(const char* s1, const char* s2);

#endif /* _STRING_H_ */

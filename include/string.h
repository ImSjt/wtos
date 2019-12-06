#ifndef _STRING_H_
#define _STRING_H_

void* memcpy(void* dest,const void* src, int size);
void memset(void* dst, char ch, int size);

void dispStr(const char* info);
void dispColorStr(const char* info, int color);
void dispInt(int input);

void delay(int time);

char* itoa(char* str, int num);
char* strcpy(char* dst, char* src);
int strlen(char* str);

#endif /* _STRING_H_ */

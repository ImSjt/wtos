#ifndef _PROTO_H_
#define _PROTO_H_
#include "type.h"
#include "proc.h"

void initTime();
void initKeyboard();
void taskTTY();

void printx(char* buf);
int sysPrintx(int unused1, int unused2, char * buf, struct Process * proc);

void portRead(u16 port, void* buf, int n);
int getTicks();

#endif /* _PROTO_H_ */

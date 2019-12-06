#ifndef _PROTO_H_
#define _PROTO_H_
#include "type.h"
#include "proc.h"

void initTime();
void initKeyboard();
void taskTTY();

void write(char* buf, int len);
int sysWrite(char* buf, int len, struct Process* proc);

#endif /* _PROTO_H_ */

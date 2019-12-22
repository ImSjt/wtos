#include "type.h"
#include "stdio.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "ipc.h"

int write(int fd, const void *buf, int count)
{
	struct Message msg;
	msg.type = WRITE;
	msg.FD   = fd;
	msg.BUF  = (void*)buf;
	msg.CNT  = count;

	ipc(BOTH, P_TASK_FS, &msg);

	return msg.CNT;
}


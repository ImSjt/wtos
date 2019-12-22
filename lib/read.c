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

int read(int fd, void* buf, int count)
{
	struct Message msg;
	msg.type = READ;
	msg.FD   = fd;
	msg.BUF  = buf;
	msg.CNT  = count;

	ipc(BOTH, P_TASK_FS, &msg);

	return msg.CNT;
}

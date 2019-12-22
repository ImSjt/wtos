#include "stdio.h"
#include "ipc.h"

int close(int fd)
{
	struct Message msg;
	msg.type   = CLOSE;
	msg.FD     = fd;

	ipc(BOTH, P_TASK_FS, &msg);

	return msg.RETVAL;
}


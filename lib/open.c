#include "ipc.h"
#include "string.h"
#include "stdio.h"
#include "ipc.h"

int open(const char* pathname, int flags)
{
	struct Message msg;

	msg.type	    = OPEN;
	msg.PATHNAME	= (void*)pathname;
	msg.FLAGS	    = flags;
	msg.NAME_LEN	= strlen(pathname);

	ipc(BOTH, P_TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.FD;
}


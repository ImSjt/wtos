#include "ipc.h"
#include "string.h"

int unlink(const char* pathname)
{
	struct Message msg;
	msg.type   = UNLINK;

	msg.PATHNAME	= (void*)pathname;
	msg.NAME_LEN	= strlen(pathname);

	ipc(BOTH, P_TASK_FS, &msg);

	return msg.RETVAL;
}


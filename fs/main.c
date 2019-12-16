#include "type.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "stdio.h"
#include "hd.h"
#include "ipc.h"

void taskFs()
{
	printf("Task FS begins.\n");

	struct Message driverMsg;
	driverMsg.type = DEV_OPEN;
	ipc(BOTH, P_TASK_HD, &driverMsg);

	spin("FS");
}

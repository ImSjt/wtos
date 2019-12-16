#include "ipc.h"
#include "stdio.h"
#include "string.h"
#include "global.h"

int ipc(int function, int obj, struct Message* msg)
{
    int ret = 0;

    if (function == RECEIVE)
        memset(msg, 0, sizeof(struct Message));

    switch (function)
    {
    case BOTH:
        ret = sendrec(SEND, obj, msg);
        ret |= sendrec(RECEIVE, obj, msg);
        break;
    case SEND:
    case RECEIVE:
        ret = sendrec(function, obj, msg);
        break;
    default:
        assert((function == BOTH) ||
               (function == SEND) || (function == RECEIVE));
        break;
    }

    return ret;
}

/* 给指定进程发送中断消息 */
void informInt(int taskNr)
{
	struct Process* p = procTable + taskNr;

	if ((p->flag & RECEIVING) &&
	    ((p->recvfrom == INTERRUPT) || (p->recvfrom == ANY)))
    {
		p->msg->source = INTERRUPT;
		p->msg->type = HARD_INT;
		p->msg = 0;
		p->hasIntMsg = 0;
		p->flag &= ~RECEIVING; /* dest has received the msg */
		p->recvfrom = NO_TASK;
		assert(p->flag == 0);
		unblock(p);

		assert(p->flag == 0);
		assert(p->msg == 0);
		assert(p->recvfrom == NO_TASK);
		assert(p->sendto == NO_TASK);
	}
	else
    {
		p->hasIntMsg = 1;
	}
}


#include "stdio.h"
#include "ipc.h"
#include "global.h"
#include "proc.h"
#include "string.h"

static int msgSend(struct Process* proc, int dest, struct Message* m);
static int msgReceive(struct Process* proc, int src, struct Message* m);

int sysSendrec(int function, int obj, struct Message* m, struct Process* proc)
{
    assert(kreenter == 0); /* 不可重入 */
    assert((obj >= 0 && obj < NR_TOTAL_PROCS) || obj == ANY || obj == INTERRUPT);

    int ret = 0;
    int caller = proc2pid(proc);
    struct Message* mla = m;

    mla->source = caller;
    assert(mla->source != obj);

    if(function == SEND)
    {
		ret = msgSend(proc, obj, m);
    }
    else if(function == RECEIVE)
    {
		ret = msgReceive(proc, obj, m);
    }
    else
    {
		panic("{sys_sendrec} invalid function: "
		      "%d (SEND:%d, RECEIVE:%d, BOCH:%d).", function, SEND, RECEIVE, BOTH);
    }

    return ret;
}

void unblock(struct Process* proc)
{
    assert(proc->flag == 0);
}

void block(struct Process* proc)
{
    assert(proc->flag);

    schedule();
}

int deadlock(int src, int dest)
{
	struct Process* p = procTable + dest;
	while(1)
    {
		if(p->flag & SENDING)
        {
			if(p->sendto == src)
            {
				/* print the chain */
				p = procTable + dest;
				printk("=_=%d", p->pid);
				do
                {
					assert(p->msg);
					p = procTable + p->sendto;
					printk("->%d", p->pid);
				}while(p != procTable + src);
				printk("=_=");

				return 1;
			}
			p = procTable + p->sendto;
		}
		else
        {
			break;
		}
	}
	return 0;
}


int msgSend(struct Process* current, int dest, struct Message* m)
{
	struct Process* sender = current;
	struct Process* pdest = procTable + dest;

//    printk("%d send to %d, %d\n", sender->pid, pdest->pid, m->type);

	assert(proc2pid(sender) != dest);

    /* 检测是否死锁 */
	if(deadlock(proc2pid(sender), dest))
    {
		panic(">>DEADLOCK<< %d->%d", sender->pid, pdest->pid);
	}

    /* 如果对方正在等待接收当前进程发送消息 */
	if ((pdest->flag & RECEIVING) &&
	    (pdest->recvfrom == proc2pid(sender) ||
	     pdest->recvfrom == ANY))
    {
		assert(pdest->msg);
		assert(m);

        memcpy(pdest->msg, m, sizeof(struct Message));
		pdest->msg = 0;
		pdest->flag &= ~RECEIVING; /* dest has received the msg */
		pdest->recvfrom = NO_TASK;
		unblock(pdest);

		assert(pdest->flag == 0);
		assert(pdest->msg == 0);
		assert(pdest->recvfrom == NO_TASK);
		assert(pdest->sendto == NO_TASK);
		assert(sender->flag == 0);
		assert(sender->msg == 0);
		assert(sender->recvfrom == NO_TASK);
		assert(sender->sendto == NO_TASK);
	}
	else
    {
		sender->flag |= SENDING;
		assert(sender->flag == SENDING);
		sender->sendto = dest;
		sender->msg = m;

		/* 将消息添加到队列中 */
		struct Process* p;
		if (pdest->sending)
        {
			p = pdest->sending;
			while (p->nextSending)
				p = p->nextSending;
			p->nextSending = sender;
		}
		else
        {
			pdest->sending = sender;
		}
		sender->nextSending = 0;

		block(sender);

		assert(sender->flag == SENDING);
		assert(sender->msg != 0);
		assert(sender->recvfrom == NO_TASK);
		assert(sender->sendto == dest);
	}

	return 0;
}

int msgReceive(struct Process* current, int src, struct Message* m)
{
	struct Process* pWhoWannaRecv = current;
	struct Process* pfrom = 0; /* from which the message will be fetched */
	struct Process* prev = 0;
	int copyok = 0;

	assert(proc2pid(pWhoWannaRecv) != src);

	if ((pWhoWannaRecv->hasIntMsg) &&
	    ((src == ANY) || (src == INTERRUPT)))
    {
		struct Message msg;
        memset(&msg, 0, sizeof(msg));
		msg.source = INTERRUPT;
		msg.type = HARD_INT;
		assert(m);

        memcpy(m, &msg, sizeof(msg));

		pWhoWannaRecv->hasIntMsg = 0;

		assert(pWhoWannaRecv->flag == 0);
		assert(pWhoWannaRecv->msg == 0);
		assert(pWhoWannaRecv->sendto == NO_TASK);
		assert(pWhoWannaRecv->hasIntMsg == 0);

		return 0;
	}

	if (src == ANY)
    {
        /* 接收队列非空 */
		if (pWhoWannaRecv->sending)
        {
			pfrom = pWhoWannaRecv->sending;
			copyok = 1;

			assert(pWhoWannaRecv->flag == 0);
			assert(pWhoWannaRecv->msg == 0);
			assert(pWhoWannaRecv->recvfrom == NO_TASK);
			assert(pWhoWannaRecv->sendto == NO_TASK);
			assert(pWhoWannaRecv->sending != 0);
			assert(pfrom->flag == SENDING);
			assert(pfrom->msg != 0);
			assert(pfrom->recvfrom == NO_TASK);
			assert(pfrom->sendto == proc2pid(pWhoWannaRecv));
		}
	}
	else
    {
		pfrom = &procTable[src];

		if ((pfrom->flag & SENDING) &&
		    (pfrom->sendto == proc2pid(pWhoWannaRecv)))
		{
			copyok = 1;

			struct Process* p = pWhoWannaRecv->sending;
			assert(p);
            
			while (p)
            {
				assert(pfrom->flag & SENDING);
				if (proc2pid(p) == src) { /* if p is the one */
					pfrom = p;
					break;
				}
				prev = p;
				p = p->nextSending;
			}

			assert(pWhoWannaRecv->flag == 0);
			assert(pWhoWannaRecv->msg == 0);
			assert(pWhoWannaRecv->recvfrom == NO_TASK);
			assert(pWhoWannaRecv->sendto == NO_TASK);
			assert(pWhoWannaRecv->sending != 0);
			assert(pfrom->flag == SENDING);
			assert(pfrom->msg != 0);
			assert(pfrom->recvfrom == NO_TASK);
			assert(pfrom->sendto == proc2pid(pWhoWannaRecv));
		}
	}

    /* 如果找到了消息，那么就从队列中拿出消息 */
	if (copyok)
    {
		if (pfrom == pWhoWannaRecv->sending)
        {
			assert(prev == 0);
			pWhoWannaRecv->sending = pfrom->nextSending;
			pfrom->nextSending = 0;
		}
		else {
			assert(prev);
			prev->nextSending = pfrom->nextSending;
			pfrom->nextSending = 0;
		}

		assert(m);
		assert(pfrom->msg);

        memcpy(m, pfrom->msg, sizeof(struct Message));

		pfrom->msg = 0;
		pfrom->sendto = NO_TASK;
		pfrom->flag &= ~SENDING;
		unblock(pfrom);
	}
	else
    {
		pWhoWannaRecv->flag |= RECEIVING;

		pWhoWannaRecv->msg = m;

		if (src == ANY)
			pWhoWannaRecv->recvfrom = ANY;
		else
			pWhoWannaRecv->recvfrom = proc2pid(pfrom);

		block(pWhoWannaRecv);

		assert(pWhoWannaRecv->flag == RECEIVING);
		assert(pWhoWannaRecv->msg != 0);
		assert(pWhoWannaRecv->recvfrom != NO_TASK);
		assert(pWhoWannaRecv->sendto == NO_TASK);
		assert(pWhoWannaRecv->hasIntMsg == 0);
	}

	return 0;
}

void dumpMsg(const char* title, struct Message* m)
{
	int packed = 0;
	printk("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
	       title,
	       (int)m,
	       packed ? "" : "\n        ",
	       procTable[m->source].name,
	       m->source,
	       packed ? " " : "\n        ",
	       m->type,
	       packed ? " " : "\n        ",
	       m->u.m3.m3i1,
	       m->u.m3.m3i2,
	       m->u.m3.m3i3,
	       m->u.m3.m3i4,
	       (int)m->u.m3.m3p1,
	       (int)m->u.m3.m3p2,
	       packed ? "" : "\n",
	       packed ? "" : "\n"/* , */
		);
}


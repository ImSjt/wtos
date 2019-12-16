#include "ipc.h"
#include "stdio.h"
#include "global.h"
#include "systask.h"
#include "string.h"

void taskSys()
{
    struct Message msg;

    while(1)
    {
        memset(&msg, 0, sizeof(msg));
        sendrec(RECEIVE, ANY, &msg);
        int src = msg.source;

        switch(msg.type)
        {
        case GET_TICKS:
        {
            msg.RETVAL = ticks;
            sendrec(SEND, src, &msg);
            break;
        }
        default:
        {
            panic("unknown msg type %d %d\n", msg.source, msg.type);
            break;
        }
        }
    }
}


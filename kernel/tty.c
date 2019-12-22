#include "keyboard.h"
#include "type.h"
#include "string.h"
#include "tty.h"
#include "global.h"
#include "irq.h"
#include "proto.h"
#include "console.h"
#include "stdio.h"
#include "ipc.h"

struct TTY ttyTable[NR_CONSOLES];
struct Console consoleTable[NR_CONSOLES];
int curConsole;

#define TTY_FIRST (ttyTable)
#define TTY_END (ttyTable + NR_CONSOLES)

static void initTTY(struct TTY* tty);
static void ttyDevRead(struct TTY* tty);
static void ttyDevWrite(struct TTY* tty);
static int isCurConsole(struct Console* con);
static void putKey(struct TTY* tty, u32 key);
static void ttyDoRead(struct TTY* tty, struct Message* msg);
static void ttyDoWrite(struct TTY* tty, struct Message* msg);



void taskTTY()
{
    struct TTY* tty;
    struct Message msg;

    initKeyboard();

    for(tty = TTY_FIRST; tty < TTY_END; ++tty)
    {
        initTTY(tty);
    }
    
    selectConsole(0);

    while(1)
    {
        for(tty = TTY_FIRST; tty < TTY_END; ++tty)
        {
            do
            {
                ttyDevRead(tty);
                ttyDevWrite(tty);
            } while(tty->inBufCount);
        }

        ipc(RECEIVE, ANY, &msg);
        int src = msg.source;
        assert(src != P_TASK_TTY);

        struct TTY* ptty = &ttyTable[msg.DEVICE];

		switch (msg.type)
        {
		case DEV_OPEN:
            memset(&msg, 0, sizeof(msg));
			msg.type = SYSCALL_RET;
			ipc(SEND, src, &msg);
			break;
		case DEV_READ:
			ttyDoRead(ptty, &msg);
			break;
		case DEV_WRITE:
			ttyDoWrite(ptty, &msg);
			break;
		case HARD_INT:
			/**
			 * waked up by clock_handler -- a key was just pressed
			 * @see clock_handler() inform_int()
			 */
			keyPressed = 0;
			continue;
		default:
			dumpMsg("TTY::unknown msg", &msg);
			break;
		}
    }
}

void inprocess(struct TTY* tty, u32 key)
{
    char output[2] = {'\0', '\0'};

    if(!(key & FLAG_EXT))
    {
        putKey(tty, key);
    }
    else
    {
        u32 rawcode = key & MASK_RAW;

        switch (rawcode)
        {
            case UP:
            {
                if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
                {
                    scrollScreen(tty->console, SCR_UP);
                }
                break;
            }
            case DOWN:
            {
                if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
                {
                    scrollScreen(tty->console, SCR_DOWN);
                }

                break;
            }
            case ENTER:
            {
                putKey(tty, '\n');
                break;
            }
            case BACKSPACE:
            {
                putKey(tty, '\b');
                break;
            }
            case F1:
            case F2:
            case F3:
            case F4:
            case F5:
            case F6:
            case F7:
            case F8:
            case F9:
            case F10:
            case F11:
            case F12:
            {
                if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
                {
                    selectConsole(rawcode - F1);
                }

                break;
            }
            default:
            {
                break;
            }
        }
    }
}

static void initTTY(struct TTY* tty)
{
    int nr;
    memset(tty, 0, sizeof(struct TTY));

    tty->inBufCount = 0;
    tty->inBufHead = tty->inBufTail = tty->inBuf;

    initScreen(tty);
}

static void ttyDevRead(struct TTY* tty)
{
    if(isCurConsole(tty->console))
    {
        keyboardRead(tty);
    }
}

#if 0
static void ttyDevWrite(struct TTY* tty)
{
    if(tty->inBufCount)
    {
        char ch = *(tty->inBufTail);
        tty->inBufTail++;
        if(tty->inBufTail == tty->inBuf + TTY_IN_BYTES)
            tty->inBufTail = tty->inBuf;

        tty->inBufCount--;
        outChar(tty->console, ch);
    }
}
#else
void ttyDevWrite(struct TTY* tty)
{
	while (tty->inBufCount)
    {
		char ch = *(tty->inBufTail);
		tty->inBufTail++;
		if (tty->inBufTail == tty->inBuf + TTY_IN_BYTES)
			tty->inBufTail = tty->inBuf;
		tty->inBufCount--;

		if (tty->ttyLeftCnt)
        {
			if (ch >= ' ' && ch <= '~')
            {
                /* printable */
				outChar(tty->console, ch);
				void * p = tty->ttyReqBuf +
					   tty->ttyTransCnt;
                memcpy(p, &ch, 1);
                tty->ttyTransCnt++;
				tty->ttyLeftCnt--;
			}
			else if (ch == '\b' && tty->ttyTransCnt)
            {
				outChar(tty->console, ch);
				tty->ttyTransCnt--;
				tty->ttyLeftCnt++;
			}

			if (ch == '\n' || tty->ttyLeftCnt == 0)
            {
				outChar(tty->console, '\n');
				struct Message msg;
				msg.type = RESUME_PROC;
				msg.PROC_NR = tty->ttyProcnr;
				msg.CNT = tty->ttyTransCnt;
				ipc(SEND, tty->ttyCaller, &msg);
				tty->ttyLeftCnt = 0;
			}
		}
        else
        {
            outChar(tty->console, ch);
        }
	}
}
#endif

static int isCurConsole(struct Console* con)
{
    return con == &consoleTable[curConsole];
}

static void putKey(struct TTY* tty, u32 key)
{
    /* 读取数据到tty缓存中 */
    if(tty->inBufCount < TTY_IN_BYTES)
    {
        *(tty->inBufHead) = key;
        tty->inBufHead++;
        if(tty->inBufHead == tty->inBuf + TTY_IN_BYTES)
            tty->inBufHead = tty->inBuf;
        tty->inBufCount++;
    }
}

#if 0
void ttyWrite(struct TTY* tty, char* buf, int len)
{
    char* p = buf;
    int i = len;

    while(i--)
    {
        outChar(tty->console, *p++);
    }
}

int sysPrintx(int unused1, int unused2, char * buf, struct Process * proc)
{
    int len = strlen(buf);

    ttyWrite(&ttyTable[proc->tty], buf, len);

    return 0;
}
#endif

int sysPrintx(int unused1, int unused2,  char* s, struct Process* proc)
{
	const char* p;
	char ch;

	char reenterErr[] = "? k_reenter is incorrect for unknown reason";
	reenterErr[0] = MAG_CH_PANIC;

	if (kreenter == 0)  /* printx() called in Ring<1~3> */
		p = s;
	else if (kreenter > 0) /* printx() called in Ring<0> */
		p = s;
	else	/* this should NOT happen */
		p = reenterErr;

	if ((*p == MAG_CH_PANIC) ||
	    (*p == MAG_CH_ASSERT && procReady < &procTable[NR_TASKS]))
    {
		disableInt();
		char * v = (char*)V_MEM_BASE;
		const char * q = p + 1; /* +1: skip the magic char */

		while (v < (char*)(V_MEM_BASE + V_MEM_SIZE))
        {
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (!*q)
            {
				while (((int)v - V_MEM_BASE) % (SCR_WIDTH * 16))
                {
					/* *v++ = ' '; */
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}

		__asm__ __volatile__("hlt");
	}

	while ((ch = *p++) != 0)
    {
		if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue; /* skip the magic char */

		outChar(ttyTable[proc->tty].console, ch);
	}

	return 0;
}

static void disp(const char* str)
{
    char ch;

    while((ch = *str++) != '\0')
        outChar(ttyTable[curConsole].console, ch);
}

int printk(const char* fmt, ...)
{
    int i;
    char buf[256];

    va_list arg = (va_list)((char*)(&fmt) + 4); /* 指向参数列表 */
    i = vsprintf(buf, fmt, arg);

    buf[i] = '\0';

    printx(buf);
    //disp(buf);

    return i;    
}

static void ttyDoRead(struct TTY* tty, struct Message* msg)
{
	/* tell the tty: */
	tty->ttyCaller   = msg->source;  /* who called, usually FS */
	tty->ttyProcnr   = msg->PROC_NR; /* who wants the chars */
	tty->ttyReqBuf   = msg->BUF;
	tty->ttyLeftCnt  = msg->CNT;    /* how many chars are requested */
	tty->ttyTransCnt = 0;           /* how many chars have been transferred */

	msg->type = SUSPEND_PROC;
	msg->CNT = tty->ttyLeftCnt;
	ipc(SEND, tty->ttyCaller, msg);
}

#define TTY_OUT_BUF_LEN 2
static void ttyDoWrite(struct TTY* tty, struct Message* msg)
{
	char buf[TTY_OUT_BUF_LEN];
	char * p = (char*)msg->BUF;
	int i = msg->CNT;
	int j;

	while (i)
    {
		int bytes = Min(TTY_OUT_BUF_LEN, i);
        memcpy(buf, (void*)p, bytes);
		for (j = 0; j < bytes; j++)
			outChar(tty->console, buf[j]);
		i -= bytes;
		p += bytes;
	}

	msg->type = SYSCALL_RET;
	ipc(SEND, msg->source, msg);
}


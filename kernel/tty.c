#include "keyboard.h"
#include "type.h"
#include "string.h"
#include "tty.h"
#include "global.h"
#include "irq.h"
#include "proto.h"
#include "console.h"

struct TTY ttyTable[NR_CONSOLES];
struct Console consoleTable[NR_CONSOLES];
int curConsole;

#define TTY_FIRST (ttyTable)
#define TTY_END (ttyTable + NR_CONSOLES)

static void initTTY(struct TTY* tty);
static void ttyDoRread(struct TTY* tty);
static void ttyDoWrite(struct TTY* tty);
static int isCurConsole(struct Console* con);

void taskTTY()
{
    struct TTY* tty;

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
            ttyDoRread(tty);
            ttyDoWrite(tty);
        }
    }
}

void inprocess(struct TTY* tty, u32 key)
{
    char output[2] = {'\0', '\0'};

    if(!(key & FLAG_EXT))
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
    else
    {
        u32 rawcode = key & MASK_RAW;

        switch (rawcode)
        {
            case UP:
            {
                if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
                {
                    disableInt();
                    outByte(CRTC_ADDR_REG, START_ADDR_H);
                    outByte(CRTC_DATA_REG, ((80*15)>>8)&0xFF);
                    outByte(CRTC_ADDR_REG, START_ADDR_L);
                    outByte(CRTC_DATA_REG, (80*15)&0xFF);
                    enableInt();
                }
                break;
            }
            case DOWN:
            {
                if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
                {

                }

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

    tty->inBufCount = 0;
    tty->inBufHead = tty->inBufTail = tty->inBuf;

    initScreen(tty);
}

static void ttyDoRread(struct TTY* tty)
{
    if(isCurConsole(tty->console))
    {
        keyboardRead(tty);
    }
}

static void ttyDoWrite(struct TTY* tty)
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

static int isCurConsole(struct Console* con)
{
    return con == &consoleTable[curConsole];
}

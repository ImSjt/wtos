#include "type.h"
#include "tty.h"
#include "global.h"
#include "irq.h"
#include "console.h"

static void setCursor(u32 position);
static void flush(struct Console* console);

void outChar(struct Console* console, char ch)
{
    u8* vmem = (u8*)(V_MEM_BASE + console->cursor * 2); /* 得到显存位置 */

    switch (ch)
    {
        case '\n': /* 换行 */
        {
            if(console->cursor < console->originalAddr + console->memLimit - SCREEN_WIDTH)
            {
                console->cursor = console->originalAddr + SCREEN_WIDTH * \
                                    ((console->cursor - console->originalAddr) / SCREEN_WIDTH + 1);
            }

            break;
        }
        case '\b': /* 删除 */
        {
            if(console->cursor > console->originalAddr)
            {
                console->cursor--;
                *(vmem-2) = ' ';
                *(vmem-1) = DEFAULT_CHAR_COLOR;
            }
            
            break;
        }
        default:
        {
            if(console->cursor < console->originalAddr + console->memLimit - 1)
            {
                *vmem++ = ch;
                *vmem++ = DEFAULT_CHAR_COLOR;
                console->cursor++;
            }
            
            break;
        }
    }

    while(console->cursor >= console->curStartAddr + SCREEN_SIZE)
    {
        scrollScreen(console, SCR_DOWN);
    }

    flush(console);
}

static void setCursor(u32 position)
{
    disableInt();

    outByte(CRTC_ADDR_REG, CURSOR_H);
    outByte(CRTC_DATA_REG, (position>>8)&0xFF);
    outByte(CRTC_ADDR_REG, CURSOR_L);
    outByte(CRTC_DATA_REG, position&0xFF);

    enableInt();
}

void initScreen(struct TTY* tty)
{
    int nr = tty - ttyTable;
    tty->console = consoleTable + nr;

    int vmemSize = V_MEM_SIZE >> 1;
    int conVMemSize = vmemSize / NR_CONSOLES;

    tty->console->originalAddr = nr * conVMemSize;
    tty->console->memLimit = conVMemSize;
    tty->console->curStartAddr = tty->console->originalAddr;
    tty->console->cursor = tty->console->originalAddr;

    if(nr == 0)
    {
        tty->console->cursor = dispPos / 2;
        dispPos = 0;
    }
    else
    {
        outChar(tty->console, nr+0x30);
        outChar(tty->console, '#');
    }

    setCursor(tty->console->cursor);
}

static void setVideoStartAddr(u32 addr)
{
    disableInt( );
    outByte(CRTC_ADDR_REG, START_ADDR_H);
    outByte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
    outByte(CRTC_ADDR_REG, START_ADDR_L);
    outByte(CRTC_DATA_REG, addr & 0xFF);
    enableInt( );
}

void selectConsole(int nr)
{
    if(nr < 0 || nr >= NR_CONSOLES)
        return;

    curConsole = nr;
    setCursor(consoleTable[nr].cursor);

    /* 设置显存起始地址 */
    setVideoStartAddr(consoleTable[nr].curStartAddr);
}

void scrollScreen(struct Console* con, int dir)
{
    if(dir == SCR_UP)
    {
        if(con->curStartAddr > con->originalAddr)
            con->curStartAddr -= SCREEN_WIDTH;
    }
    else if(dir == SCR_DOWN)
    {
        if(con->curStartAddr + SCREEN_SIZE < con->originalAddr + con->memLimit)
            con->curStartAddr += SCREEN_WIDTH;
    }
    else
    {

    }

    setCursor(con->cursor);

    if(con == &consoleTable[curConsole])
        setVideoStartAddr(con->curStartAddr);
}

static void flush(struct Console* console)
{
    setCursor(console->cursor);

    if(console == &consoleTable[curConsole])
        setVideoStartAddr(console->curStartAddr);
}

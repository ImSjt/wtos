#include "type.h"
#include "tty.h"
#include "global.h"
#include "irq.h"
#include "console.h"

static void setCursor(u32 position);

void outChar(struct Console* console, char ch)
{
    u8* vmem = (u8*)(V_MEM_BASE + console->cursor * 2);

    *vmem++ = ch;
    *vmem++ = DEFAULT_CHAR_COLOR;
    console->cursor++;

    setCursor(console->cursor);    
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

    setVideoStartAddr(con->curStartAddr);
    setCursor(con->cursor);
}

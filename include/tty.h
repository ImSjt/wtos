#ifndef _TTY_H_
#define _TTY_H_
#include "type.h"

#define CRTC_ADDR_REG 0x3D4 /* CRT Controller Registers - Addr Register */
#define CRTC_DATA_REG 0x3D5 /* CRT Controller Registers - Data Register */
#define START_ADDR_H 0xC /* reg index of video mem start addr (MSB) */
#define START_ADDR_L 0xD /* reg index of video mem start addr (LSB) */
#define CURSOR_H 0xE /* reg index of cursor position (MSB) */
#define CURSOR_L 0xF /* reg index of cursor position (LSB) */
#define V_MEM_BASE 0xB8000 /* base of color video memory */
#define V_MEM_SIZE 0x8000 /* 32K: B8000H -> BFFFFH */

/* tty输入缓存 */
#define TTY_IN_BYTES 256
#define NR_CONSOLES 3

struct Console;

struct TTY
{
    u32 inBuf[TTY_IN_BYTES]; /* 输入缓存区 */
    u32* inBufHead; /* 指向缓存区下一个空闲的位置 */
    u32* inBufTail; /* 指向需要处理的数据 */
    int inBufCount; /* 缓存区中的数据量 */

    struct Console* console;    /* 指向对应的控制台 */
};

struct Console
{
    u32 curStartAddr;   /* 当前显示的位置 */
    u32 originalAddr;       /* 当前控制台对应的显存起始位置 */
    u32 memLimit;           /* 当前控制台的显存大小 */
    u32 cursor;             /* 当前光标位置 */
};

void inprocess(struct TTY* tty, u32 key);

#endif /* _TTY_H_ */

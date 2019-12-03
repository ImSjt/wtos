#include "string.h"
#include "irq.h"
#include "protect.h"
#include "type.h"
#include "keyboard.h"
#include "keymap.h"
#include "proto.h"
#include "tty.h"
#include "console.h"

static struct KeyboardBuf kbBuf;

static int codeWithE0;
static int shiftL; /* l shift state */
static int shiftR; /* r shift state */
static int altL; /* l alt state */
static int altR; /* r left state */
static int ctrlL; /* l ctrl state */ 
static int ctrlR; /* l ctrl state */
static int capsLock; /* Caps Lock */
static int numLock; /* Num Lock */
static int scrollLock; /* Scroll Lock */
static int column;

static u8 getByteFormKBuf();

static void keyboardHandler(int irq)
{
	u8 code = inByte(KB_DATA);

    /* 写入环形缓冲区中 */
	if (kbBuf.count < KB_IN_BYTES)
    {
		*(kbBuf.head) = code;
		kbBuf.head++;
		if (kbBuf.head == kbBuf.buf + KB_IN_BYTES)
        {
			kbBuf.head = kbBuf.buf;
		}
		kbBuf.count++;
	}
}

void initKeyboard()
{
    kbBuf.count = 0;
    kbBuf.head = kbBuf.tail = kbBuf.buf;

    codeWithE0 = 0;
    shiftL = 0; /* l shift state */
    shiftR = 0; /* r shift state */
    altL = 0; /* l alt state */
    altR = 0; /* r left state */
    ctrlL = 0; /* l ctrl state */ 
    ctrlR = 0; /* l ctrl state */
    capsLock = 0; /* Caps Lock */
    numLock = 0; /* Num Lock */
    scrollLock = 0; /* Scroll Lock */
    column = 0;

    putIrqHandler(KEYBOARD_IRQ, keyboardHandler);
    enableIrq(KEYBOARD_IRQ);
}

void keyboardRead(struct TTY* tty)
{
	u8	code;
	char output[2];
	int	make;	/* TRUE: make;  FALSE: break. */
    u32 key = 0;
    u32* keyrow;

	memset(output, 0, 2);

	code = getByteFormKBuf();

	/* 下面开始解析扫描码 */
	if(code == 0xE1)
    {
		/* 暂时不做任何操作 */
	}
	else if(code == 0xE0)
    {
        codeWithE0 = 1;
	}
	else
    {   /* 下面处理可打印字符 */

		/* 首先判断Make Code 还是 Break Code */
		make = (code & FLAG_BREAK ? 0 : 1);

        keyrow = &keymap[(code&0x7F) * MAP_COLS];
        column = 0;

        if(shiftL || shiftR)
            column  = 1;

        if(codeWithE0)
        {
            column = 2;
            codeWithE0 = 0;
        }

        key =  keyrow[column];

        switch(key)
        {
        case SHIFT_L:
            shiftL = make;
            break;
        case SHIFT_R:
            shiftR = make;
            break;
        case CTRL_L:
            ctrlL = make;
            break;
        case CTRL_R:
            ctrlR = make;
            break;
        case ALT_L:
            altL = make;
            break;
        case ALT_R:
            altR = make;
            break;
        default:
            break;
        }

        if(make)
        {
             key |= shiftL ? FLAG_SHIFT_L : 0;
             key |= shiftR ? FLAG_SHIFT_R : 0;
             key |= ctrlL  ? FLAG_CTRL_L  : 0;
             key |= ctrlR  ? FLAG_CTRL_R  : 0;
             key |= altL   ? FLAG_ALT_L   : 0;
             key |= altR   ? FLAG_ALT_R   : 0;

             inprocess(tty, key);
        }
	}
}

static u8 getByteFormKBuf()
{
    u8 code;

    while(kbBuf.count <= 0) {}

	disableInt();

    code = *(kbBuf.tail);
    kbBuf.tail++;
    if(kbBuf.tail == kbBuf.buf + KB_IN_BYTES)
    {
        kbBuf.tail = kbBuf.buf;
    }
    kbBuf.count--;

    enableInt();

    return code;
}

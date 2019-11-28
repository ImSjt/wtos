#include "string.h"
#include "irq.h"
#include "protect.h"
#include "type.h"
#include "keyboard.h"
#include "keymap.h"

static struct KeyboardBuf kbBuf;

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

    putIrqHandler(KEYBOARD_IRQ, keyboardHandler);
    enableIrq(KEYBOARD_IRQ);
}

void keyboardRead()
{
	u8	code;
	char output[2];
	int	make;	/* TRUE: make;  FALSE: break. */

	memset(output, 0, 2);

	if(kbBuf.count > 0)
    {
		//disableInt();
		code = *(kbBuf.tail);
		kbBuf.tail++;
		if(kbBuf.tail == kbBuf.buf + KB_IN_BYTES)
        {
			kbBuf.tail = kbBuf.buf;
		}
		kbBuf.count--;
		//enableInt();
        
		/* 下面开始解析扫描码 */
		if(code == 0xE1)
        {
			/* 暂时不做任何操作 */
		}
		else if(code == 0xE0)
        {
			/* 暂时不做任何操作 */
		}
		else
        {   /* 下面处理可打印字符 */

			/* 首先判断Make Code 还是 Break Code */
			make = (code & FLAG_BREAK ? 0 : 1);

			/* 如果是Make Code 就打印，是 Break Code 则不做处理 */
			if(make)
            {
				output[0] = keymap[(code&0x7F)*MAP_COLS];
				dispStr(output);
			}
		}
        
		/* disp_int(scan_code); */
	}
}

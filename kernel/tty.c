#include "keyboard.h"
#include "type.h"
#include "string.h"
#include "tty.h"
#include "global.h"
#include "irq.h"

void taskTTY()
{
    while(1)
    {
        keyboardRead();
    }
}

void inprocess(u32 key)
{
    char output[2] = {'\0', '\0'};

    if(!(key & FLAG_EXT))
    {
        output[0] = key & 0xFF;
        dispStr(output);

        /* 修改光标位置 */
        outByte(CRTC_ADDR_REG, CURSOR_H);
        outByte(CRTC_DATA_REG, ((dispPos/2)>>8)&0xFF);
        outByte(CRTC_ADDR_REG, CURSOR_L);
        outByte(CRTC_DATA_REG, (dispPos/2)&0xFF);
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
            default:
            {
                break;
            }
        }
    }
}


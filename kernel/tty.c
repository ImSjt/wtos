#include "keyboard.h"

void taskTTY()
{
    while(1)
    {
        keyboardRead();
    }
}

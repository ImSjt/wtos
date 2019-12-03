#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */

void outChar(struct Console* console, char ch);
void initScreen(struct TTY* tty);
void selectConsole(int nr);

#endif /* _CONSOLE_H_ */

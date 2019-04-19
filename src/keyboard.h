#ifndef KEYBOARD_H
#define KEYBOARD_H

extern void initializeKeyboard();
extern void setKeyboardInterrupts(char on);
extern unsigned char getKey();

extern void keyboardISR(int interrupt, int error, void *data);

#endif
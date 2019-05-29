#ifndef KEYBOARD_H
#define KEYBOARD_H

extern void initializeKeyboard();
extern unsigned char getKey();

extern char getChar();

extern void keyboardISR(int interrupt, int error, void *data);

#endif
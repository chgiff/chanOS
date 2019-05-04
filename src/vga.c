

#include "vga.h"
#include "memory.h"
#include "interrupt.h"
#include "serial.h"
#include <stdarg.h>

#define VGA_BASE 0xb8000
static unsigned short *vgaBuff = (unsigned short*)VGA_BASE;
static int width = 80;
static int height = 25;
static unsigned int cursor = 0;

//TODO debugging mirror to serial
static int writeToSerial = 1;

void VGA_display_char(char c)
{
    int lines;
    int i;
    int enableInterrupts;


    enableInterrupts = clearIntConditional();

    if (c == '\n') {
        cursor = ((cursor/width) + 1) * width;
        if (cursor >= width*height){
            lines = cursor/width - height + 1;
            memmove1((void*)vgaBuff, (void*)(&vgaBuff[lines*width]), (height-lines)*width*2);
            for(i = (height-lines)*width; i < width*height; i ++){
                vgaBuff[i] = 0x0720;
            }
            cursor -= (lines * width);
        }
    }
    else if (c == '\r'){
        cursor = (cursor/width) * width;
    }
    else if (c == '\b'){
        if((cursor % width) > 0){
            cursor--;
            vgaBuff[cursor] = ' ';
        }
    }
    else {
        vgaBuff[cursor] = 0x0700 | c;
        if((cursor % width) < (width - 1)){
            cursor++;
        }
    }
    
    if(cursor >= width*height) cursor = 0;

    setIntConditional(enableInterrupts);
}


int VGA_display_str_internal(const char *str)
{
    int i;
    for(i = 0; str[i]; i++){
        VGA_display_char(str[i]);
    }
    return i;
}
void VGA_display_str(const char *str)
{
    VGA_display_str_internal(str);
}

void VGA_clear()
{
    int i;
    for(i = 0; i < width*height; i ++){
        vgaBuff[i] = 0x0720;
    }
}
 

void generic_display_char(char c)
{
    if(writeToSerial){
        SER_write_char(c);
    }
    VGA_display_char(c);
}
int generic_display_str_internal(const char *str)
{
    int i;
    for(i = 0; str[i]; i++){
        generic_display_char(str[i]);
    }
    return i;
}

int printHex_internal(unsigned long long num)
{
    if(num == 0) return 0;
    int c = printHex_internal(num >> 4);
    
    if((num & 0xF) < 10) generic_display_char('0' + (num & 0xF));
    else generic_display_char('A' + (num & 0xF) - 10);

    return c + 1;
}
int printHex(unsigned long long num)
{
    generic_display_char('0');
    generic_display_char('x');
    if(num == 0){
        generic_display_char('0');
        return 3;
    }
    return 2 + printHex_internal(num);
}

int printUnsigned_internal(unsigned long long num)
{
    if(num == 0) return 0;
    int c = printUnsigned_internal(num/10);
    generic_display_char('0' + (num%10));
    return c+1;
}
int printUnsigned(unsigned long long num)
{
    if(num == 0){
        generic_display_char('0');
        return 1;
    }
    return printUnsigned_internal(num);
}

int printSigned(long long num)
{
    int c = 0;
    if(num < 0){
        generic_display_char('-');
        num *= -1;
        c = 1;
    }
    return c + printUnsigned(num);
}

int printk(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int printedChars = 0;
    int i;

    for(i = 0; fmt[i]; i++){
        if(fmt[i] == '%'){
            i++;
            switch(fmt[i]){
                case 'c':
                    generic_display_char(va_arg(args, int));
                    printedChars++;
                    break;
                case 'p':
                    printedChars += printHex((unsigned long long)va_arg(args, void*));
                    break;
                case 'd':
                    printedChars += printSigned(va_arg(args, int));
                    break;
                case 'u':
                    printedChars += printUnsigned(va_arg(args, unsigned int));
                    break;
                case 'x':
                    VGA_display_str("0x");
                    printedChars += printHex(va_arg(args, unsigned int));
                    break;
                case 'h':
                    i++;
                    if(fmt[i] == 'd') printedChars += printSigned(va_arg(args, int));
                    else if(fmt[i] == 'u') printedChars += printUnsigned(va_arg(args, unsigned int));
                    else if(fmt[i] == 'x') printedChars += printHex(va_arg(args, unsigned int));
                    else return -1; //error
                    break;
                case 'l':
                    i++;
                    if(fmt[i] == 'd') printedChars += printSigned(va_arg(args, long));
                    else if(fmt[i] == 'u') printedChars += printUnsigned(va_arg(args, unsigned long));
                    else if(fmt[i] == 'x') printedChars += printHex(va_arg(args, unsigned long));
                    else return -1; //error
                    break;
                case 'q':
                    i++;
                    if(fmt[i] == 'd') printedChars += printSigned(va_arg(args, long long));
                    else if(fmt[i] == 'u') printedChars += printUnsigned(va_arg(args, unsigned long long));
                    else if(fmt[i] == 'x') printedChars += printHex(va_arg(args, unsigned long long));
                    else return -1; //error
                    break;

                case 's':
                    printedChars += generic_display_str_internal(va_arg(args, char*));
                    break;
                default:
                    generic_display_char(fmt[i]);
                    printedChars++;
                    break;
            }
        }
        else{
            generic_display_char(fmt[i]);
            printedChars++;
        }
    }

    va_end(args);

    return printedChars;
}


#include "vga.h"
#include "memory.h"
#include "interrupt.h"
#include <stdarg.h>

#define VGA_BASE 0xb8000
static unsigned short *vgaBuff = (unsigned short*)VGA_BASE;
static int width = 80;
static int height = 25;
static unsigned int cursor = 0;

void VGA_display_char(char c)
{
    int lines;
    int i;
    int enableInterupts = 0;


    if(areInterruptsEnabled()){
        enableInterupts = 1;
        CLI;
    }

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

    if(enableInterupts){
        STI;
    }
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

int printHex(unsigned long long num)
{
    if(num == 0) return 0;
    int c = printHex(num >> 4);
    
    if((num & 0xF) < 10) VGA_display_char('0' + (num & 0xF));
    else VGA_display_char('A' + (num & 0xF) - 10);

    return c + 1;
}

int printUnsigned(unsigned long long num)
{
    if(num == 0) return 0;
    int c = printUnsigned(num/10);
    VGA_display_char('0' + (num%10));
    return c+1;
}

int printSigned(long long num)
{
    int c = 0;
    if(num < 0){
        VGA_display_char('-');
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
                    VGA_display_char(va_arg(args, int));
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
                    printedChars += VGA_display_str_internal(va_arg(args, char*));
                    break;
                default:
                    VGA_display_char(fmt[i]);
                    printedChars++;
                    break;
            }
        }
        else{
            VGA_display_char(fmt[i]);
            printedChars++;
        }
    }

    va_end(args);

    return printedChars;
}
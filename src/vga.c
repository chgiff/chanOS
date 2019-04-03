

#include "vga.h"
#include "memory.h"

#define VGA_BASE 0xb8000
const unsigned int vga_base = 0xb8000;
//static unsigned short *vgaBuff = (unsigned short*)VGA_BASE;
static int width = 80;
static int height = 20;
static unsigned int cursor = 0;
//static unsigned char color = FG(VGA_LIGHT_GREY) | BG(VGA_BLACK);
static unsigned short color = 0x0700;

void VGA_display_char(char c)
{
    if (c == '\n') {
        cursor = ((cursor/width) + 1) * width;
        //if (cursor >= width*height)
        //    scroll();
    }
    else if (c == '\r')
        cursor = (cursor/width);
    else {
        unsigned int addr = VGA_BASE + cursor*2;
        //addr += cursor*2;
        *((unsigned short*)addr) = (color << 8) | c;
        if((cursor % width) < (width - 1)){
            cursor++;
        }
    }
    
    if(cursor >= width*height) cursor = 0;
}

void VGA_clear()
{
    unsigned int addr = VGA_BASE;
    for(int i = 0; i < width*height; i ++){
        *((unsigned short*)addr) = color | 0x20;
        addr += 2;
    }
}
#include "vga.h"
#include "keyboard.h"

#define WHITE_TXT 0x07
//static unsigned int *vgaBuff = (unsigned int*)0xb8000;

void kmain()
{
    //char loop = 0;
    //while(!loop);
    


    VGA_clear();

    VGA_display_str("hello world\ndddd");


    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('h');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');
    VGA_display_char('\n');

    VGA_display_char('l');
    VGA_display_char('l');

    VGA_display_str("blahblahblahblakshdfhas;lkjsd;alfkj\n;alksjdf ashhahaha");

    printk("Testing signed: %d\nTesting unsigned: %u\nTesting hex:%x\n", -369, 7739, 0x48AC5);

    long long blah = 982374987234234;
    printk("Testing long long: %qu\n", blah);

    short sh = -45;
    printk("Testing signed short: %hd\n", sh);


    initializeKeyboard();
    while(1){
        printk("%c", getKey());
    }
}
#include "vga.h"
#include "keyboard.h"
#include "interrupt.h"
#include "gdt.h"
#include "serial.h"

#define WHITE_TXT 0x07

void wait()
{
    int i = 900000;
    while(i) i--;
}

void kmain()
{
    //char loop = 0;
    //while(!loop);

    init_gdt();

    IRQ_set_handler(33, &keyboardISR, (void *)0); //irq 1
    initializeKeyboard();

    IRQ_set_handler(36, &serial_write_isr, (void *)0);
    SER_init();

    IRQ_init();

    VGA_clear();

    VGA_display_str("hello world\ndddd");
    SER_write_str("Hello serial!!!", 15);

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

    VGA_display_char('-');

   
    while(1){
        asm("hlt");
    }
}
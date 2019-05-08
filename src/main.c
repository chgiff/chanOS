#include "vga.h"
#include "keyboard.h"
#include "interrupt.h"
#include "serial.h"
#include "gdt.h"
#include "multiboot.h"
#include "test.h"
#include "paging.h"
#include "memory.h"

#define WHITE_TXT 0x07

void wait()
{
    int i = 900000;
    while(i) i--;
}

void kmain(void *multibootInfo)
{
    //char loop = 0;
    //while(!loop);

    setup_identity();

    /* Initialization code */
    //gdt
    init_gdt();

    //memory
    parseBootTags(multibootInfo);
    IRQ_set_handler(0xE, &pageFaultISR, (void*)0);

    IRQ_set_handler(33, &keyboardISR, (void *)0); //irq 1 for keyboard
    initializeKeyboard();

    IRQ_set_handler(36, &serial_write_isr, (void *)0);//irq 4 for com1
    SER_init();

    IRQ_init();

    VGA_clear();
    /* End Initialization code*/


    printk("Loaded by: %s\n", getBootLoaderName());

    test_pf_allocator();
   
   /*
    void *blah = kmalloc(15);
    printk("Allocated memory: %p\n", blah);
    void *blah2 = kmalloc(1115);
    printk("Allocated memory: %p\n", blah2);
    */

    while(1){
        asm("hlt");
    }
}
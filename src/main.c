#include "vga.h"
#include "keyboard.h"
#include "interrupt.h"
#include "serial.h"
#include "gdt.h"
#include "multiboot.h"
#include "test.h"
#include "paging.h"
#include "memory.h"
#include "multitask.h"

#include "snakes.h"

#define PAUSE 0

void test_thread(void *arg)
{
    printk("test thread: %p\n", arg);
    yield();
    printk("test thread print 2: %p\n", arg);
}

void init(void *multibootInfo)
{   
    /* Initialization code */
    //page table
    setup_identity();

    //gdt
    init_gdt();

    //memory
    parseBootTags(multibootInfo);
    IRQ_set_handler(0xE, &pageFaultISR, (void*)0);

    IRQ_set_handler(33, &keyboardISR, (void *)0); //irq 1 for keyboard
    initializeKeyboard();

    IRQ_set_handler(36, &serial_write_isr, (void *)0);//irq 4 for com1
    SER_init();

    //turn on interrupts
    IRQ_init();

    VGA_clear();
    printk("Loaded by: %s\n", getBootLoaderName());
    /* End Initialization code*/
}

void printLoop(void *args)
{
    while(1){
        printk("%c", getChar());
    }
}

void kmain(void *multibootInfo)
{
    char loop = !PAUSE;
    while(!loop);

    init(multibootInfo);

    //test_pf_allocator();
    //test_virtual_allocator();
    
    
    // char *blah = (char*)kmalloc(15);
    // printk("Allocated memory: %p\n", blah);
    // void *blah2 = kmalloc(1115);
    // printk("Allocated memory: %p\n", blah2);
    
    // blah[0] = 'h';
    // blah[1] = 'e';
    // blah[2] = 'l';
    // blah[3] = 'l';
    // blah[4] = 'o';
    // printk("String: %s\n", (char*)blah);

    PROC_create_kthread(test_thread, (void*)2);
    PROC_create_kthread(test_thread, (void*)87126);
    PROC_create_kthread(printLoop, (void*)0);

    setup_snakes(1);

    while(1){
        PROC_run();
        asm("hlt");
    }
    printk("Done!\n");
}
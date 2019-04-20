#include "interrupt.h"
#include "memory.h"
#include "asm/interrupt_gen.h"
#include "stdint.h"
#include "vga.h"
#include "gdt.h"

//PIC #defines
#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define PIC_EOI		0x20

#define PIC1_BASE 0x20
#define PIC2_BASE 0x2F

#define IDT_SIZE 256

#define FLAGS_INTERRUPT_BIT (1 << 9)

struct IDT_element
{
    uint16_t funcPtrLower;
    uint16_t gdtSelector;
    
    uint8_t stackTableIndex:3;
    uint8_t reserved1:5;
    uint8_t interruptGate:1; //should be 0
    uint8_t ones:3; //should be all 1s
    uint8_t zero:1; //should be 0
    uint8_t dpl:2;
    uint8_t present:1;

    uint16_t funcPtrMid;
    uint32_t funcPtrUpper;
    uint32_t reserved2;
}__attribute__((packed));
struct IDT_element IDT[IDT_SIZE];

struct IDT_ptr {
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));
struct IDT_ptr idtp;

struct IRQ_handler_element {
    irq_handler_t function;
    void *data;
};
struct IRQ_handler_element irq_handlers[IDT_SIZE];

extern void idt_load();

char areInterruptsEnabled()
{
    uint64_t flag;
    asm volatile ( "pushf; pop %0" : "=a"(flag));
    if(flag & FLAGS_INTERRUPT_BIT) return 1;
    return 0;
}

inline char clearIntConditional()
{
    if(areInterruptsEnabled()){
        CLI;
        return 1;
    }
    return 0;
}

inline void setIntConditional(char cond)
{
    if(cond){
        STI;
    }
}

void PIC_remap(int offset1, int offset2)
{
	unsigned char a1, a2;
 
	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);
 
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);
}

void IRQ_init()
{
    int i;
    CLI;

    //setup pic
    PIC_remap(PIC1_BASE, PIC2_BASE);

    for(i = 0; i < IDT_SIZE; i++){
        uint64_t handlerAddr = (uint64_t)irq_address_list[i];
        IDT[i].funcPtrLower = handlerAddr & 0xFFFF;
        IDT[i].funcPtrMid = handlerAddr >> 16;
        IDT[i].funcPtrUpper = handlerAddr >> 32;

        IDT[i].gdtSelector = getCodeOffset();
        IDT[i].stackTableIndex = getTssInterruptStack();
        IDT[i].reserved1 = 0; //TODO
        IDT[i].interruptGate = 0;

        IDT[i].ones = 0x7;
        IDT[i].zero = 0;
        IDT[i].dpl = 0;
        IDT[i].present = 1;
        IDT[i].reserved2 = 0; //TODO
    }

    //special exceptions that have their own stack
    IDT[0x8].stackTableIndex = getTssDFStack(); //double fault
    IDT[0xD].stackTableIndex = getTssGPStack(); //general protection fault
    IDT[0xE].stackTableIndex = getTssPFStack(); //page fault

    
    idtp.base = (uint64_t)&IDT;
    idtp.limit = (uint16_t)sizeof(struct IDT_element)*IDT_SIZE - 1;

    //call IDT instruction
    idt_load();

    IRQ_set_mask(0); //turn off the timer

    STI;
}

void IRQ_set_mask(int irq)
{
    uint16_t port;
    uint8_t value;
 
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void IRQ_clear_mask(int irq)
{
    uint16_t port;
    uint8_t value;
 
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

int IRQ_get_mask(int irq)
{
    uint16_t port;
    uint8_t value;
 
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = (inb(port) >> irq) & 0x1;
    return value;
}

void IRQ_end_of_interrupt(int irq)
{
    if(irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
 
	outb(PIC1_COMMAND, PIC_EOI);
}

void IRQ_set_handler(int irq, irq_handler_t handler, void *arg)
{
    irq_handlers[irq].function = handler;
    irq_handlers[irq].data = arg;
}

void isr_c(uint64_t interrupt, uint64_t errorCode)
{
    //call handler
    if(irq_handlers[interrupt].function != 0){
        irq_handlers[interrupt].function(interrupt, errorCode, irq_handlers[interrupt].data);
    }
    else{
        printk("Unhandled interupt: %lu\n", interrupt);
        asm("hlt");
    }

    //ack
    if(interrupt >= PIC1_BASE && interrupt < (PIC1_BASE + 8)){
        IRQ_end_of_interrupt(interrupt - PIC1_BASE);
    }
    else if(interrupt >= PIC2_BASE && interrupt < (PIC2_BASE + 8)){
        IRQ_end_of_interrupt(interrupt - PIC2_BASE + 8);
    }
}
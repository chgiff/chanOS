#include "interupt.h"
#include "stdint.h"

struct IDT_element
{
    uint16_t funcPtrLower;
    uint16_t gdtSelector;
    
    uint8_t stackTableIndex:4;
    uint8_t reserved1:4;
    uint8_t interuptGate:1; //should be 0
    uint8_t ones:3; //should be all 1s
    uint8_t zero:1; //should be 0
    uint8_t dpl:2;
    uint8_t present:1;

    uint16_t funcPtrMid;
    uint32_t funcPtrUpper;
    uint32_t reserved2;
}__attribute__((packed));

struct IDT_element IDT[256];

void IRQ_init()
{


    //TODO call IDT instruction
}

void IRQ_set_mask(int irq)
{

}

void IRQ_clear_mask(int irq)
{

}

int IRQ_get_mask(int IRQline)
{
    return 0;
}

void IRQ_end_of_interrupt(int irq)
{

}

void IRQ_set_handler(int irq, irq_handler_t handler, void *arg)
{
    uint64_t handlerAddr = (uint64_t)handler;
    IDT[irq].funcPtrLower = handlerAddr & 0xFFFF;
    IDT[irq].funcPtrMid = handlerAddr >> 16;
    IDT[irq].funcPtrUpper = handlerAddr >> 32;

    IDT[irq].gdtSelector = 0; //TODO

    IDT[irq].stackTableIndex = 0; //TODO
    IDT[irq].reserved1 = 0; //TODO
    IDT[irq].interuptGate = 0;
    IDT[irq].ones = 0x7;
    IDT[irq].zero = 0;
    IDT[irq].dpl = 0; //TODO
    IDT[irq].present = 0; //TODO

    IDT[irq].reserved2 = 0; //TODO
}
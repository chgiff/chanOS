#include "interupt.h"
#include "asm/interupt_gen.h"
#include "stdint.h"

#define IDT_SIZE

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
struct IDT_element IDT[IDT_SIZE];

struct IDT_ptr {
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));

void IRQ_init()
{
    int i;
    for(i = 0; i < IDT_SIZE; i++){
        uint64_t handlerAddr = (uint64_t)irq_address_list[i];
        IDT[i].funcPtrLower = handlerAddr & 0xFFFF;
        IDT[i].funcPtrMid = handlerAddr >> 16;
        IDT[i].funcPtrUpper = handlerAddr >> 32;

        IDT[i].gdtSelector = 0; //TODO
        IDT[i].stackTableIndex = 0; //TODO
        IDT[i].reserved1 = 0; //TODO
        IDT[i].interuptGate = 0;

        IDT[i].ones = 0x7;
        IDT[i].zero = 0;
        IDT[i].dpl = 0; //TODO
        IDT[i].present = 1;
        IDT[i].reserved2 = 0; //TODO
    }

    struct IDT_ptr ptr;
    ptr.base = (uint64_t)&IDT;
    ptr.limit = (uint16_t)sizeof(struct IDT_element)*IDT_SIZE;
    uint64_t IDT_ptr_ptr = (uint64_t)&ptr;

    //TODO call IDT instruction
    volatile asm ("lidt [%0]"::(IDT_ptr_ptr));
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
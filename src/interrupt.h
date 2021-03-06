#ifndef INTERRUPT_H
#define INTERRUPT_H

#define CLI asm("cli;")
#define STI asm("sti;")
extern char areInterruptsEnabled();
extern char clearIntConditional();
extern void setIntConditional(char cond);

extern void IRQ_init(void);
extern void IRQ_set_mask(int irq);
extern void IRQ_clear_mask(int irq);
extern int IRQ_get_mask(int irq);
extern void IRQ_end_of_interrupt(int irq);

typedef void (*irq_handler_t)(int, int, void*);
extern void IRQ_set_handler(int irq, irq_handler_t handler, void *arg);

#endif
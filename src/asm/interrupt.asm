global isr_normal
global isr_error
global idt_load

extern isr_c
extern idtp

idt_load:
    lidt [idtp]
    ret

isr_normal:
    push rsi
    call isr_c
    pop rsi
    pop rdi
    iretq

isr_error:
    call isr_c
    pop rsi
    pop rdi
    iretq
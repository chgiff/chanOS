global isr_normal

extern isr_c

isr_normal:
    push rsi
    call isr_c
    pop rsi
    pop rdi
    iret

isr_error:
    call isr_c
    pop rsi
    pop rdi
    iret
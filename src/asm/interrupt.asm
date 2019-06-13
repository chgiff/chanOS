global isr_normal
global isr_error
global idt_load
global switch_contexts

extern isr_c
extern idtp
extern memcpy1
extern curr_proc
extern next_proc

switch_contexts:
    cmp rsi, 0
    je load_next_context

    pop rsi
    pop rdi

    ;push gs
    ;push fs
    ;push es
    ;push ds
    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rdx
    push rcx
    push rbx
    push rax

    ;memcpy: dest, src, size (stack -> curr_proc)
    mov rdi, [curr_proc]
    mov rsi, rsp
    mov rdx, 160 ;set based on number of regs saved
    call memcpy1

    push rdi
    push rsi

load_next_context:
    pop rsi
    pop rdi

    ;memcpy: dest, src, size (next_proc -> stack)
    mov rdi, rsp
    mov rsi, [next_proc]
    mov rdx, 160 ;set based on number of regs saved
    call memcpy1

    ;update process
    mov rsi, [next_proc]
    mov [curr_proc], rsi

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ;pop ds
    ;pop es
    ;pop fs
    ;pop gs

    iretq

idt_load:
    lidt [idtp]
    ret

isr_normal:
    push rsi

    push r10
    push r11
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9
    push rax
    call isr_c
    pop rax
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r11
    pop r10

    mov rsi, [curr_proc]
    mov rdi, [next_proc]
    cmp rsi, rdi
    jne switch_contexts
    pop rsi
    pop rdi
    iretq

isr_error:
    push r10
    push r11
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9
    push rax
    call isr_c
    pop rax
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r11
    pop r10

    pop rsi
    pop rdi
    add rsp, 8

    push rsi
    push rdi
    mov rsi, [curr_proc]
    mov rdi, [next_proc]
    cmp rsi, rdi
    jne switch_contexts
    pop rsi
    pop rdi 
    iretq
global long_mode_start
extern kmain

global gdt_load
extern gdtp

section .text
bits 64
long_mode_start:
    ; load 0 into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call kmain

    hlt

gdt_load:
    lgdt [gdtp]
    jmp .flush   ; 0x08 is the offset to our code segment: Far jump!
.flush:
   ret
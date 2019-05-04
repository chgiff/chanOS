FILENAME=$(dirname $0)"/interrupt_gen.asm"

echo "extern isr_normal"  > $FILENAME
echo "extern isr_error" >> $FILENAME

for((i=0; i<=255; i++));
do
    echo "global irq$i" >> $FILENAME
done

#handle error interrupts
i=8
    echo "irq$i:" >> $FILENAME

    # echo "mov dword [0xb8000], 0x4f524f45" >> $FILENAME
    # echo "mov dword [0xb8004], 0x4f3a4f52" >> $FILENAME
    # echo "mov dword [0xb8008], 0x4f204f45" >> $FILENAME
    # echo "mov dword [0xb800C], 0x4f464f44" >> $FILENAME
    # echo "cli" >> $FILENAME
    # echo "hlt" >> $FILENAME

    echo "   push rdi" >> $FILENAME
    echo "   push rsi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   mov rsi, [rsp+16]" >> $FILENAME
    echo "   jmp isr_error" >> $FILENAME
for((i=10; i<=14; i++));
do
    echo "irq$i:" >> $FILENAME


    # echo "mov dword [0xb8000], 0x4f524f45" >> $FILENAME
    # echo "mov dword [0xb8004], 0x4f3a4f52" >> $FILENAME
    # echo "mov dword [0xb8008], 0x4f204f20" >> $FILENAME
    # echo "mov dword [0xb800C], 0x4f504f47" >> $FILENAME
    # echo "cli" >> $FILENAME
    # echo "hlt" >> $FILENAME

    echo "   push rdi" >> $FILENAME
    echo "   push rsi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   mov rsi, [rsp+16]" >> $FILENAME
    echo "   jmp isr_error" >> $FILENAME
done

#handle normal interrupts
for((i=0; i<=7; i++));
do
    echo "irq$i:" >> $FILENAME

    # echo "mov dword [0xb8000], 0x4f524f45" >> $FILENAME
    # echo "mov dword [0xb8004], 0x4f3a4f52" >> $FILENAME
    # echo "mov dword [0xb8008], 0x4f204f52" >> $FILENAME
    # echo "mov dword [0xb8030], 0x4f524f45" >> $FILENAME
    # echo "hlt" >> $FILENAME


    echo "   push rdi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   jmp isr_normal" >> $FILENAME
done
for((i=9; i<=9; i++));
do
    echo "irq$i:" >> $FILENAME


    # echo "mov dword [0xb8000], 0x4f524f45" >> $FILENAME
    # echo "mov dword [0xb8004], 0x4f3a4f52" >> $FILENAME
    # echo "mov dword [0xb8008], 0x4f204f3a" >> $FILENAME
    # echo "mov dword [0xb8040], 0x4f524f45" >> $FILENAME
    # echo "hlt" >> $FILENAME

    echo "   push rdi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   jmp isr_normal" >> $FILENAME
done
for((i=15; i<=255; i++));
do
    echo "irq$i:" >> $FILENAME


    # echo "mov dword [0xb8000], 0x4f524f45" >> $FILENAME
    # echo "mov dword [0xb8004], 0x4f3a4f52" >> $FILENAME
    # echo "mov dword [0xb8008], 0x4f204f3a" >> $FILENAME
    # echo "mov dword [0xb800c], 0x4f204f3a" >> $FILENAME
    #echo "hlt" >> $FILENAME

    echo "   push rdi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   jmp isr_normal" >> $FILENAME
done

#generate the header file for c
FILENAME=$(dirname $0)"/interrupt_gen.h"
echo "#ifndef INTERRUPT_GEN_H" > $FILENAME
echo "#define INTERRUPT_GEN_H" >> $FILENAME

for((i=0; i<=255; i++));
do
    echo "extern void irq$i();" >> $FILENAME
done
echo "void* irq_address_list[256] = {" >> $FILENAME
for((i=0; i<=254; i++));
do
    echo "&irq$i," >> $FILENAME
done
echo "&irq255 };" >> $FILENAME

echo "#endif" >> $FILENAME
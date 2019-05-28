FILENAME=$(dirname $0)"/interrupt_gen.asm"

echo "extern isr_normal"  > $FILENAME
echo "extern isr_error" >> $FILENAME
echo "extern sysCallISR" >> $FILENAME
echo "extern curr_proc" >> $FILENAME
echo "extern next_proc" >> $FILENAME
echo "extern switch_contexts" >> $FILENAME

for((i=0; i<=255; i++));
do
    echo "global irq$i" >> $FILENAME
done

#handle error interrupts
i=8
    echo "irq$i:" >> $FILENAME
    echo "   push rdi" >> $FILENAME
    echo "   push rsi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   mov rsi, [rsp+16]" >> $FILENAME
    echo "   jmp isr_error" >> $FILENAME
for((i=10; i<=14; i++));
do
    echo "irq$i:" >> $FILENAME
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
    echo "   push rdi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   jmp isr_normal" >> $FILENAME
done
for((i=9; i<=9; i++));
do
    echo "irq$i:" >> $FILENAME
    echo "   push rdi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   jmp isr_normal" >> $FILENAME
done
for((i=15; i<=127; i++));
do
    echo "irq$i:" >> $FILENAME
    echo "   push rdi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   jmp isr_normal" >> $FILENAME
done

#handle sys call trap
for((i=128; i<=128; i++));
do
    echo "irq$i:" >> $FILENAME
    echo "   call sysCallISR" >> $FILENAME
    echo "   push rdi" >> $FILENAME
    echo "   push rsi" >> $FILENAME
    echo "   mov rsi, [curr_proc]" >> $FILENAME
    echo "   mov rdi, [next_proc]" >> $FILENAME
    echo "   cmp rsi, rdi" >> $FILENAME
    echo "   jne switch_contexts" >> $FILENAME
    echo "   pop rsi" >> $FILENAME
    echo "   pop rdi" >> $FILENAME
    echo "   iretq" >> $FILENAME
done

for((i=129; i<=255; i++));
do
    echo "irq$i:" >> $FILENAME
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
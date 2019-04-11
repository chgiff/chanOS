FILENAME="interupt_gen.asm"

echo "extern isr_normal"  > $FILENAME

#handle error interupts
i=8
    echo "irq$i:" >> $FILENAME
    echo "   push rdi" >> $FILENAME
    echo "   push rsi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   mov rsi, [rsp+12]" #TODO sp+12
    echo "   jmp isr_error" >> $FILENAME
for((i=10; i<=14; i++));
do
    echo "irq$i:" >> $FILENAME
    echo "   push rdi" >> $FILENAME
    echo "   push rsi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   mov rsi, [rsp+12]" #TODO sp+12
    echo "   jmp isr_error" >> $FILENAME
done

#handle normal interupts
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
for((i=15; i<=255; i++));
do
    echo "irq$i:" >> $FILENAME
    echo "   push rdi" >> $FILENAME
    echo "   mov rdi, $i" >> $FILENAME
    echo "   jmp isr_normal" >> $FILENAME
done

#generate the header file for c
FILENAME=interupt_gen.h
echo "#ifndef INTERUPT_GEN_H" > $FILENAME
echo "#define INTERUPT_GEN_H" >> $FILENAME

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
#include "gdt.h"

#define TSS_TYPE_NOT_BUSY 9
#define TSS_TYPE_BUSY 11

//seperate stack info
#define TSS_INTERRUPT_STACK_INDEX 1
#define TSS_INTERRUPT_STACK_SIZE 4096
#define TSS_GP_STACK_INDEX 2
#define TSS_GP_STACK_SIZE 4096
#define TSS_DF_STACK_INDEX 3
#define TSS_DF_STACK_SIZE 4096
#define TSS_PF_STACK_INDEX 4
#define TSS_PF_STACK_SIZE 4096

#define OPERAND_SIZE_32 1
#define OPERAND_SIZE_16 0

// #define TYPE_CODE 0xA //1010 (read only)
#define TYPE_CODE 0x8 //1000 (r/w)
#define TYPE_DATA 0x2

#define DT_TSS 0
#define DT_OTHER 1

#define GDT_ELEMENTS 4

struct GDT_descriptor
{
   uint16_t limit_low;    
   uint16_t base_low;        
   uint8_t  base_middle;
   
   uint8_t  type:4;
   uint8_t  dt:1;
   uint8_t  protection:2;
   uint8_t  present:1;        
   
   uint8_t  limit_high:4;
   uint8_t  avl:1;
   uint8_t  code_64bit:1;
   uint8_t  operand_size:1;
   uint8_t  granularity:1;
   
   uint8_t  base_high;        
} __attribute__((packed));

struct GDT_descriptor_long
{
   struct GDT_descriptor base_descriptor;

   //specific to long descriptor
   uint32_t base_higher;    
   uint8_t reserved1;
   uint8_t zero;
   uint16_t reserved2;  
} __attribute__((packed));

struct TSS_structure{
    uint32_t reserved1;
    
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint64_t reserved2;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t reserved3;

    uint16_t reserved4;
    uint16_t io_map_base_addr;

}__attribute__((packed));

struct GDT_ptr {
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));
struct GDT_ptr gdtp;

struct GDT_descriptor GDT[GDT_ELEMENTS];
struct TSS_structure TSS;

//setup seperate stacks
uint8_t TSS_interrupt_stack[TSS_INTERRUPT_STACK_SIZE]; //normal interrupts
uint8_t TSS_GP_stack[TSS_GP_STACK_SIZE]; //general protection fault
uint8_t TSS_DF_stack[TSS_DF_STACK_SIZE]; //double fault
uint8_t TSS_PF_stack[TSS_PF_STACK_SIZE]; //page faults

extern void gdt_load();

void set_descriptor(struct GDT_descriptor *descriptor_ptr, uint64_t base, uint32_t limit, 
    uint8_t operand_size, uint8_t type, uint8_t dt, uint8_t code_64bit)
{
   descriptor_ptr->base_low    = (base & 0xFFFF);
   descriptor_ptr->base_middle = (base >> 16) & 0xFF;
   descriptor_ptr->base_high   = (base >> 24) & 0xFF;

   descriptor_ptr->limit_low   = (limit & 0xFFFF);
   descriptor_ptr->limit_high = (limit >> 16) & 0x0F;
   descriptor_ptr->avl = 0;
   descriptor_ptr->code_64bit = code_64bit;
   descriptor_ptr->operand_size = operand_size;
   descriptor_ptr->granularity = 0;

   descriptor_ptr->type = type & 0xF;
   descriptor_ptr->dt = dt;
   descriptor_ptr->protection = 0;
   descriptor_ptr->present = 1;
}

void set_tss(struct GDT_descriptor_long *descriptor_ptr, uint64_t base, uint32_t limit)
{
    set_descriptor((struct GDT_descriptor *)descriptor_ptr, base, limit, 0, TSS_TYPE_NOT_BUSY, DT_TSS, 0); //TODO type

    descriptor_ptr->base_higher = base >> 32;
    descriptor_ptr->reserved1 = 0;
    descriptor_ptr->zero = 0;
    descriptor_ptr->reserved2 = 0;
}

void init_gdt()
{
    set_descriptor(&GDT[0], 0, 0, 0, 0, 0, 0); //empty
    set_descriptor(&GDT[1], 0, 0xFFFFFFFF, OPERAND_SIZE_16, TYPE_CODE, DT_OTHER, 1); //code
    set_tss((struct GDT_descriptor_long *)&GDT[2], (uint64_t)&TSS, sizeof(TSS)); //tss takes up 2 spaces

    gdtp.base = (uint64_t)&GDT;
    gdtp.limit = (uint16_t)sizeof(struct GDT_descriptor)*GDT_ELEMENTS - 1;

    gdt_load();

    TSS.ist1 = ((uint64_t)&(TSS_interrupt_stack)) + TSS_INTERRUPT_STACK_SIZE;
    TSS.ist2 = ((uint64_t)&TSS_GP_stack) + TSS_GP_STACK_SIZE;
    TSS.ist3 = ((uint64_t)&TSS_DF_stack) + TSS_DF_STACK_SIZE;
    TSS.ist4 = ((uint64_t)&TSS_PF_stack) + TSS_PF_STACK_SIZE;
    uint8_t offset = getTSSOffset();
    asm("ltr %w0" :: "r"(offset));
}

uint64_t getCodeOffset()
{
    return (uint64_t)((void *)(&GDT[1]) - (void *)(&GDT[0]));
}

uint64_t getTSSOffset()
{
    return (uint64_t)((void *)(&GDT[2]) - (void *)(&GDT[0]));
}

uint8_t getTssInterruptStack()
{
    return TSS_INTERRUPT_STACK_INDEX;
}
uint8_t getTssGPStack()
{
    return TSS_GP_STACK_INDEX;
}
uint8_t getTssDFStack()
{
    return TSS_DF_STACK_INDEX;
}
uint8_t getTssPFStack()
{
    return TSS_PF_STACK_INDEX;
}
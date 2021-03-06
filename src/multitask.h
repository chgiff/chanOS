#ifndef MULTITASK_H
#define MULTITASK_H

#include <stdint.h>

struct ProcessQueue{
    struct Process *head;
};

struct Process{
    //context
    uint64_t rax, rbx, rcx, rdx, rdi, rsi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rbp;
    //uint64_t ds, es;
    //uint64_t fs, gs;
    //these must be last in this order to match interupt stack values
    uint64_t rip, cs, rflags, rsp, ss;

    //other
    void *stack_start;
    int pid;

    //list pointers
    struct Process *nextProc, *prevProc;
    struct ProcessQueue *currQueue;
    struct Process *nextInQueue, *prevInQueue;
    
}__attribute__((packed));

extern struct Process *curr_proc;

extern void PROC_run();

typedef void (*kproc_t)(void*);
extern struct Process * PROC_create_kthread(kproc_t entry_point, void* arg);

extern void PROC_reschedule();

extern void PROC_block_on(struct ProcessQueue *queue, int enable_ints);
extern void PROC_unblock_head(struct ProcessQueue *queue);
extern void PROC_unblock_all(struct ProcessQueue *queue);
extern void PROC_init_queue(struct ProcessQueue *queue);

//system call 1
static inline void yield()
{
    asm volatile ( "mov $1, %%rdi; INT $0x80"::: "rdi" );
}

//system call 2
static inline void kexit()
{
    asm volatile ( "mov $2, %%rdi; INT $0x80"::: "rdi" );
}

#endif
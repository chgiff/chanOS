#include "multitask.h"
#include "memory.h"
#include "vga.h"

#define STACK_SIZE 2*1024*1024

struct Process *curr_proc, *next_proc;
struct Process *all_procs;


struct Process dummy_process = { .next=&dummy_process, .prev=&dummy_process };
struct Process *all_procs = &dummy_process;

void runSchedule()
{
    next_proc = curr_proc->next;
    if(next_proc == &dummy_process) next_proc = next_proc->next;
}

void PROC_run()
{
    curr_proc = &dummy_process;
    yield();
}

struct Process * PROC_create_kthread(kproc_t entry_point, void* arg)
{
    static int pid=1;
    
    struct Process *newProc = (struct Process *)kmalloc(sizeof(struct Process));
    newProc->pid = pid++;
    
    //setup stack
    newProc->stack_start = kmalloc(STACK_SIZE);
    newProc->rsp = (uint64_t)newProc->stack_start + STACK_SIZE - 16;
    *((uint64_t*)(newProc->rsp)) = (uint64_t)kexit; //return address on stack

    //setup other registers
    newProc->rip = (uint64_t)entry_point;
    asm ("mov %%cs, %0":"=r"(newProc->cs));
    asm ("pushf; mov (%%rsp), %0; popf":"=r"(newProc->rflags));
    asm ("mov %%ss, %0":"=r"(newProc->ss));
    newProc->rdi = (uint64_t)arg;

    //add into process circular list
    newProc->prev = all_procs->prev;
    all_procs->prev->next = newProc;
    all_procs->prev = newProc;
    newProc->next = all_procs;

    return newProc;
}

void PROC_reschedule()
{
    runSchedule();
}


//system call 1
void yieldSysCall()
{
    runSchedule();
}

//system call 2
void kexitSysCall(void)
{
    struct Process *removed = curr_proc;
    removed->next->prev = removed->prev;
    removed->prev->next = removed->next;
    runSchedule();

    kfree(removed->stack_start);
    kfree(removed);
    curr_proc = (struct Process *)0;
}

void sysCallISR(int call_number, void * arg1, void* arg2, void* arg3, void* arg4, void* arg5)
{
    switch (call_number)
    {
    case 1:
        yieldSysCall();
        break;
    
    case 2:
        kexitSysCall();
        break;

    default:
        break;
    }
}
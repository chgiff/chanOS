#include "multitask.h"
#include "memory.h"
#include "vga.h"
#include "interrupt.h"

#define STACK_SIZE 2*1024*1024

struct Process *curr_proc, *next_proc;
struct Process dummy_process = { .nextProc=&dummy_process, .prevProc=&dummy_process };
struct Process *all_procs = &dummy_process;

struct ProcessQueue readyQueue = {.head=0};

void runSchedule()
{
    if(curr_proc->currQueue == &readyQueue){
        next_proc = curr_proc->nextInQueue;
    }
    else if(readyQueue.head != 0){
        next_proc = readyQueue.head;
    }
    else{
        next_proc = &dummy_process;
    }
}

void unlinkFromQueue(struct Process *proc)
{
    if(proc->nextInQueue == proc){
        proc->currQueue->head = 0;
    }
    else{
        proc->nextInQueue->prevInQueue = proc->prevInQueue;
        proc->prevInQueue->nextInQueue = proc->nextInQueue;
        if(proc->currQueue->head == proc){
            proc->currQueue->head = proc->nextInQueue;
        }
    }
    proc->currQueue = 0;
}

void appendToQueue(struct Process *newProc, struct ProcessQueue *queue)
{
    newProc->currQueue = queue;
    if(queue->head == 0){
        newProc->nextInQueue = newProc;
        newProc->prevInQueue = newProc;
        queue->head = newProc;
    }
    else{
        newProc->prevInQueue = queue->head->prevInQueue;
        queue->head->prevInQueue->nextInQueue = newProc;
        queue->head->prevInQueue = newProc;
        newProc->nextInQueue = queue->head;
    }
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
    newProc->prevProc = all_procs->prevProc;
    all_procs->prevProc->nextProc = newProc;
    all_procs->prevProc = newProc;
    newProc->nextProc = all_procs;

    //add to ready queue
    appendToQueue(newProc, &readyQueue);

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
    removed->nextProc->prevProc = removed->prevProc;
    removed->prevProc->nextProc = removed->nextProc;

    unlinkFromQueue(removed);

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



void PROC_block_on(struct ProcessQueue *queue, int enable_ints)
{
    if(!queue){
        return;
    }

    unlinkFromQueue(curr_proc);
    appendToQueue(curr_proc, queue);

    if(enable_ints){
        STI;
    }

    yield();
}

void PROC_unblock_head(struct ProcessQueue *queue)
{
    struct Process *proc = queue->head;
    unlinkFromQueue(proc);
    appendToQueue(proc, &readyQueue);
}

void PROC_unblock_all(struct ProcessQueue *queue)
{
    while(queue->head){
        PROC_unblock_head(queue);
    }
}

void PROC_init_queue(struct ProcessQueue *queue)
{
    queue->head = 0;
}
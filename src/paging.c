#include "paging.h"
#include "multiboot.h"
#include "vga.h"
#include "memory.h"
#include <stdint.h>

#define PAGE_SIZE 4096

//linked list of frames
struct FrameHeader {
    uint64_t size;
    struct FrameHeader *next;
};
struct FrameHeader *availableFrames = 0;

void * nextVirtualPage = (void*)0x20000000000;

struct P4_entry{
    uint16_t present:1;
    uint16_t rw:1;
    uint16_t us:1;
    uint16_t pwt:1;
    uint16_t pcd:1;
    uint16_t a:1;
    uint16_t ign:1;
    uint16_t mbz:2;
    uint16_t avl:3;
    uint16_t p3_addr_low_4:4;
    uint32_t p3_addr_mid_32;
    uint16_t p3_addr_high_4:4;
    uint16_t available:11;
    uint16_t nx:1;
}__attribute__((packed));
struct P3_entry{
    uint16_t present:1;
    uint16_t rw:1;
    uint16_t us:1;
    uint16_t pwt:1;
    uint16_t pcd:1;
    uint16_t a:1;
    uint16_t ign:1;
    uint16_t big_page:1;
    uint16_t mbz:1;
    uint16_t allocated:1;
    uint16_t avl:2;
    uint16_t p2_addr_low_4:4;
    uint32_t p2_addr_mid_32;
    uint16_t p2_addr_high_4:4;
    uint16_t available:11;
    uint16_t nx:1;
}__attribute__((packed));
struct P2_entry{
    uint16_t present:1;
    uint16_t rw:1;
    uint16_t us:1;
    uint16_t pwt:1;
    uint16_t pcd:1;
    uint16_t a:1;
    uint16_t ign1:1;
    uint16_t big_page:1;
    uint16_t ign2:1;
    uint16_t allocated:1;
    uint16_t avl:2;
    uint16_t p1_addr_low_4:4;
    uint32_t p1_addr_mid_32;
    uint16_t p1_addr_high_4:4;
    uint16_t available:11;
    uint16_t nx:1;
}__attribute__((packed));
struct P1_entry{
    uint16_t present:1;
    uint16_t rw:1;
    uint16_t us:1;
    uint16_t pwt:1;
    uint16_t pcd:1;
    uint16_t a:1;
    uint16_t d:1;
    uint16_t pat:1;
    uint16_t g:1;
    uint16_t allocated:1;
    uint16_t avl:2;
    uint16_t phys_addr_low_4:4;
    uint32_t phys_addr_mid_32;
    uint16_t phys_addr_high_4:4;
    uint16_t available:11;
    uint16_t nx:1;
}__attribute__((packed));

struct P4_entry P4[512] __attribute__((aligned (4096)));

struct P3_entry P3_identity[512] __attribute__((aligned (4096)));

void setup_identity()
{
    int i;
    //point first P4 entry to the P3 identity table
    *(uint64_t*)&P4[0] = (uint64_t)(&P3_identity);
    P4[0].present = 1;
    P4[0].rw = 1; //writable
    
    for(i = 0; i < 512; i++){
        uint64_t addr = 0x40000000 * i;
        *(uint64_t*)&P3_identity[i] = addr;
        P3_identity[i].present = 1;
        P3_identity[i].rw = 1;
        P3_identity[i].big_page = 1;
    }

    asm ("mov %0, %%cr3" :: "r"(&P4));
}

struct P1_entry * getPageTableL1(void * virtualPointer, struct P4_entry *pageTable)
{
    uint64_t l4Index, l3Index, l2Index, l1Index;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    if(!pageTable[l4Index].present){
        //error
        return 0;
    }

    struct P3_entry *l3 = (struct P3_entry *)(((uint64_t*)pageTable)[l4Index] & ~0xFFF);
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    if(!l3[l3Index].present){
        //error
        return 0;
    }
    if(l3[l3Index].big_page){
        //1 GB page case
        return 0;
    }

    struct P2_entry *l2 = (struct P2_entry *)(((uint64_t*)l3)[l3Index] & ~0xFFF);
    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1F; //bits 21-29
    if(!l2[l2Index].present){
        //error
        return 0;
    }
    if(l2[l2Index].big_page){
        //2 MB page case
        return 0;
    }

    struct P1_entry *l1 = (struct P1_entry *)(((uint64_t*)l2)[l2Index] & ~0xFFF);
    l1Index = (((uint64_t)virtualPointer) >> 12) & 0x1F; //bits 12-20
    return &l1[l1Index];
}

uint64_t getPhysicalAddress(void * virtualPointer, struct P4_entry *pageTable)
{
    uint64_t l4Index, l3Index, l2Index, l1Index;
    uint64_t physAddr;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    if(!pageTable[l4Index].present){
        //error
        return 0;
    }

    struct P3_entry *l3 = (struct P3_entry *)(((uint64_t*)pageTable)[l4Index] & ~0xFFF);
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    if(!l3[l3Index].present){
        //error
        return 0;
    }
    if(l3[l3Index].big_page){
        //1 GB page case
        physAddr = ((uint64_t)virtualPointer) & 0x3FFFFFFF; //bits 0-29 offset
        physAddr |= (((uint64_t*)l3)[l3Index] & ~0x3FFFFFFF);
        return physAddr;
    }

    struct P2_entry *l2 = (struct P2_entry *)(((uint64_t*)l3)[l3Index] & ~0xFFF);
    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1F; //bits 21-29
    if(!l2[l2Index].present){
        //error
        return 0;
    }
    if(l2[l2Index].big_page){
        //2 MB page case
        physAddr = ((uint64_t)virtualPointer) & 0x1FFFFF; //bits 0-20 offset
        physAddr |= (((uint64_t*)l2)[l2Index] & ~0x1FFFFF);
        return physAddr;
    }

    struct P1_entry *l1 = (struct P1_entry *)(((uint64_t*)l2)[l2Index] & ~0xFFF);
    l1Index = (((uint64_t)virtualPointer) >> 12) & 0x1F; //bits 12-20
    if(!l1[l1Index].present){
        //error
        return 0;
    }
    physAddr = ((uint64_t)virtualPointer) & 0xFFF; //bits 0-11 offset
    physAddr |= (((uint64_t*)l1)[l1Index] & ~0xFFF);
    return physAddr;
}

//sets virtual address page as unavailble and returns physical address
void* deallocate(void * virtualPointer, struct P4_entry *pageTable)
{
    uint64_t l4Index, l3Index, l2Index, l1Index;
    uint64_t physAddr;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    if(!pageTable[l4Index].present){
        //error
        return 0;
    }

    struct P3_entry *l3 = (struct P3_entry *)(((uint64_t*)pageTable)[l4Index] & ~0xFFF);
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    if(!l3[l3Index].present){
        //error
        return 0;
    }
    if(l3[l3Index].big_page){
        //1 GB page case
        physAddr = ((uint64_t)virtualPointer) & 0x3FFFFFFF; //bits 0-29 offset
        physAddr |= (((uint64_t*)l3)[l3Index] & ~0x3FFFFFFF);
        return (void*)physAddr;
    }

    struct P2_entry *l2 = (struct P2_entry *)(((uint64_t*)l3)[l3Index] & ~0xFFF);
    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1F; //bits 21-29
    if(!l2[l2Index].present){
        //error
        return 0;
    }
    if(l2[l2Index].big_page){
        //2 MB page case
        physAddr = ((uint64_t)virtualPointer) & 0x1FFFFF; //bits 0-20 offset
        physAddr |= (((uint64_t*)l2)[l2Index] & ~0x1FFFFF);
        return (void*)physAddr;
    }

    struct P1_entry *l1 = (struct P1_entry *)(((uint64_t*)l2)[l2Index] & ~0xFFF);
    l1Index = (((uint64_t)virtualPointer) >> 12) & 0x1F; //bits 12-20
    if(!l1[l1Index].present){
        //error
        return 0;
    }
    l1[l1Index].available = 0;
    physAddr = ((uint64_t)virtualPointer) & 0xFFF; //bits 0-11 offset
    physAddr |= (((uint64_t*)l1)[l1Index] & ~0xFFF);
    return (void*)physAddr;
}

void setAllocated(void * virtualPointer, struct P4_entry *pageTable)
{
    uint64_t l4Index, l3Index, l2Index, l1Index;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    if(!pageTable[l4Index].present){
        void *newPage = MMU_pf_alloc();
        memset1(newPage, 0, PAGE_SIZE);
        *(uint64_t*)(&pageTable[l4Index]) = (uint64_t)newPage;
        pageTable[l4Index].present = 1;
        pageTable[l4Index].rw = 1;
    }

    struct P3_entry *l3 = (struct P3_entry *)(((uint64_t*)pageTable)[l4Index] & ~0xFFF);
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    if(!l3[l3Index].present){
        void *newPage = MMU_pf_alloc();
        memset1(newPage, 0, PAGE_SIZE);
        *(uint64_t*)(&l3[l3Index]) = (uint64_t)newPage;
        l3[l3Index].present = 1;
        l3[l3Index].allocated = 1;
        l3[l3Index].rw = 1;
    }
    if(l3[l3Index].big_page){
        //1 GB page case
        return;
    }

    struct P2_entry *l2 = (struct P2_entry *)(((uint64_t*)l3)[l3Index] & ~0xFFF);
    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1F; //bits 21-29
    if(!l2[l2Index].present){
        void *newPage = MMU_pf_alloc();
        memset1(newPage, 0, PAGE_SIZE);
        *(uint64_t*)(&l2[l2Index]) = (uint64_t)newPage;
        l2[l2Index].present = 1;
        l2[l2Index].allocated = 1;
        l2[l2Index].rw = 1;
    }
    if(l2[l2Index].big_page){
        //2 MB page case
        return;
    }

    struct P1_entry *l1 = (struct P1_entry *)(((uint64_t*)l2)[l2Index] & ~0xFFF);
    l1Index = (((uint64_t)virtualPointer) >> 12) & 0x1F; //bits 12-20
    l1[l1Index].allocated = 1;
    //l1 is set available but the page isn't allocated yet
}

void *MMU_pf_alloc()
{
    if(availableFrames){
        void *pf = (void *)availableFrames;
        availableFrames = availableFrames->next;
        return pf;
    }

    return getNewPage();
}

void MMU_pf_free(void *pf)
{
    pf = (void*)(((uint64_t)pf + PAGE_SIZE-1) & ~(PAGE_SIZE-1));

    struct FrameHeader *frameHeader = (struct FrameHeader *)pf;
    frameHeader->size = availableFrames->size + 1;
    frameHeader->next = availableFrames;
    availableFrames = frameHeader;
}

void *MMU_alloc_page()
{
    void *ret = nextVirtualPage;
    setAllocated(nextVirtualPage, P4);
    nextVirtualPage += PAGE_SIZE;
    return ret;
}

void *MMU_alloc_pages(int num)
{
    int i;
    void *ret = nextVirtualPage;
    for(i = 0; i < num; i++){
        setAllocated(nextVirtualPage, P4);
        nextVirtualPage += PAGE_SIZE;
    }
    return ret;
}

void MMU_free_page(void *virtualAddress)
{
    void *physAddr = deallocate(virtualAddress, P4);
    MMU_pf_free(physAddr);
}

void MMU_free_pages(void *virtualAddress, int num)
{
    int i;
    for(i = 0; i < num; i++){
        MMU_free_page(virtualAddress);
        virtualAddress += PAGE_SIZE;
    }
}

void printPageFaultError(uint64_t virtualAddr, uint64_t pageTableAddr, int errCode)
{
    printk("Page fault error (%d) at address %lx using table %lx\n", errCode, virtualAddr, pageTableAddr);
}

void pageFaultISR(int interrupt, int error, void *data)
{
    uint64_t virtualAddr, pageTableAddr;
    asm ("mov %%cr2, %0": "=r"(virtualAddr));
    asm ("mov %%cr3, %0": "=r"(pageTableAddr));

    struct P1_entry *l1 = getPageTableL1((void*)virtualAddr, P4);
    if(l1 == 0){
        // printk("error getting page table\n");
        // asm ("hlt");
        return;
    }
    
    if(l1->allocated){
        if(l1->present){
            printk("this shouldn't happen\n");
            asm ("hlt");
        }
        void *backingPage = MMU_pf_alloc();
        memset1(backingPage, 0, PAGE_SIZE);
        *(uint64_t *)l1 = ((uint64_t)backingPage & ~(0xFFF));
        l1->present = 1;
        l1->allocated = 1;
        l1->rw = 1;
    }
    else{
        printPageFaultError(virtualAddr, pageTableAddr, error);
        asm ("hlt");
    }
}
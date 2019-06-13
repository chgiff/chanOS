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
    uint16_t allocated:1;
    uint16_t avl:2;
    uint16_t p3_addr_low_4:4;
    uint32_t p3_addr_mid_32;
    uint16_t p3_addr_high_4:4;
    uint16_t available:11;
    uint16_t nx:1;
}__attribute__((packed));
struct PageTable_L4{
    struct P4_entry entries[512];    
};
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
struct PageTable_L3{
    struct P3_entry entries[512];
};
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
struct PageTable_L2{
    struct P2_entry entries[512];
};
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
struct PageTable_L1{
    struct P1_entry entries[512];
};

struct PageTable_L4 P4 __attribute__((aligned (4096)));

struct PageTable_L3 P3_identity __attribute__((aligned (4096)));

void setup_identity()
{
    int i;
    //point first P4 entry to the P3 identity table
    *(uint64_t*)&P4.entries[0] = (uint64_t)(&P3_identity);
    P4.entries[0].present = 1;
    P4.entries[0].rw = 1; //writable
    
    for(i = 0; i < 512; i++){
        uint64_t addr = 0x40000000 * i;
        *(uint64_t*)&P3_identity.entries[i] = addr;
        P3_identity.entries[i].present = 1;
        P3_identity.entries[i].rw = 1;
        P3_identity.entries[i].big_page = 1;
    }

    asm ("mov %0, %%cr3" :: "r"(&P4));
}

struct PageTable_L1 * getPageTableL1(void * virtualPointer, struct PageTable_L4 *l4)
{
    uint64_t l4Index, l3Index, l2Index;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    struct P4_entry *l4_entry = &l4->entries[l4Index];
    if(!l4_entry->present){
        //error
        return 0;
    }
    struct PageTable_L3 *l3 = (struct PageTable_L3 *)((*((uint64_t *)l4_entry)) & ~0xFFF);
    
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    struct P3_entry *l3_entry = &l3->entries[l3Index];
    if(!l3_entry->present){
        //error
        return 0;
    }
    if(l3_entry->big_page){
        //1GB page case
        return 0;
    }
    struct PageTable_L2 *l2 = (struct PageTable_L2 *)((*((uint64_t *)l3_entry)) & ~0xFFF);

    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1FF; //bits 21-29
    struct P2_entry *l2_entry = &l2->entries[l2Index];
    if(!l2_entry->present){
        //error
        return 0;
    }
    if(l2_entry->big_page){
        //2MB page case
        return 0;
    }
    struct PageTable_L1 *l1 = (struct PageTable_L1 *)((*((uint64_t *)l2_entry)) & ~0xFFF);

    return l1;
}

uint64_t getPhysicalAddress(void * virtualPointer, struct PageTable_L4 *l4)
{
    uint64_t l4Index, l3Index, l2Index, l1Index;
    uint64_t physAddr;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    struct P4_entry *l4_entry = &l4->entries[l4Index];
    if(!l4_entry->present){
        //error
        return 0;
    }
    struct PageTable_L3 *l3 = (struct PageTable_L3 *)((*((uint64_t *)l4_entry)) & ~0xFFF);
    
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    struct P3_entry *l3_entry = &l3->entries[l3Index];
    if(!l3_entry->present){
        //error
        return 0;
    }
    if(l3_entry->big_page){
        //1GB page case
        physAddr = ((uint64_t)virtualPointer) & 0x3FFFFFFF; //bits 0-29 offset
        physAddr |= (((uint64_t)l3_entry) & ~0x3FFFFFFF);
        return physAddr;
    }
    struct PageTable_L2 *l2 = (struct PageTable_L2 *)((*((uint64_t *)l3_entry)) & ~0xFFF);

    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1FF; //bits 21-29
    struct P2_entry *l2_entry = &l2->entries[l2Index];
    if(!l2_entry->present){
        //error
        return 0;
    }
    if(l2_entry->big_page){
        //2 MB page case
        physAddr = ((uint64_t)virtualPointer) & 0x1FFFFF; //bits 0-20 offset
        physAddr |= (((uint64_t)l2_entry) & ~0x1FFFFF);
        return physAddr;
    }
    struct PageTable_L1 *l1 = (struct PageTable_L1 *)((*((uint64_t *)l2_entry)) & ~0xFFF);
    
    l1Index = (((uint64_t)virtualPointer) >> 12) & 0x1FF; //bits 12-20
    struct P1_entry *l1_entry = &l1->entries[l1Index];
    if(!l1_entry->present){
        //error
        return 0;
    }
    physAddr = ((uint64_t)virtualPointer) & 0xFFF; //bits 0-11 offset
    physAddr |= (*((uint64_t*)l1_entry) & ~0xFFF);
    return physAddr;
}

//sets virtual address page as unavailble and returns physical address
void* deallocate(void * virtualPointer, struct PageTable_L4 *l4)
{
    uint64_t l4Index, l3Index, l2Index, l1Index;
    uint64_t physAddr;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    struct P4_entry *l4_entry = &l4->entries[l4Index];
    if(!l4_entry->present){
        //error
        return (void*)0;
    }
    struct PageTable_L3 *l3 = (struct PageTable_L3 *)((*((uint64_t *)l4_entry)) & ~0xFFF);
    
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    struct P3_entry *l3_entry = &l3->entries[l3Index];
    if(!l3_entry->present){
        //error
        return (void*)0;
    }
    if(l3_entry->big_page){
        //1GB page case
        physAddr = ((uint64_t)virtualPointer) & 0x3FFFFFFF; //bits 0-29 offset
        physAddr |= (((uint64_t)l3_entry) & ~0x3FFFFFFF);
        return (void*)physAddr;
    }
    struct PageTable_L2 *l2 = (struct PageTable_L2 *)((*((uint64_t *)l3_entry)) & ~0xFFF);

    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1FF; //bits 21-29
    struct P2_entry *l2_entry = &l2->entries[l2Index];
    if(!l2_entry->present){
        //error
        return (void*)0;
    }
    if(l2_entry->big_page){
        //2 MB page case
        physAddr = ((uint64_t)virtualPointer) & 0x1FFFFF; //bits 0-20 offset
        physAddr |= (((uint64_t)l2_entry) & ~0x1FFFFF);
        return (void*)physAddr;
    }
    struct PageTable_L1 *l1 = (struct PageTable_L1 *)((*((uint64_t *)l2_entry)) & ~0xFFF);
    
    l1Index = (((uint64_t)virtualPointer) >> 12) & 0x1FF; //bits 12-20
    struct P1_entry *l1_entry = &l1->entries[l1Index];
    if(!l1_entry->present){
        //error
        return (void*)0;
    }
    l1_entry->available = 0;

    physAddr = ((uint64_t)virtualPointer) & 0xFFF; //bits 0-11 offset
    physAddr |= (*((uint64_t*)l1_entry) & ~0xFFF);
    return (void*)physAddr;
}

void setAllocated(void * virtualPointer, struct PageTable_L4 *l4)
{
    uint64_t l4Index, l3Index, l2Index, l1Index;
    l4Index = (((uint64_t)virtualPointer) >> 39) & 0x1FF; //bits 39-48
    struct P4_entry *l4_entry = &l4->entries[l4Index];
    if(!l4_entry->present){
        //create frame for l3 table
        void *newPage = MMU_pf_alloc();
        memset1(newPage, 0, PAGE_SIZE);
        *(uint64_t*)(l4_entry) = (uint64_t)newPage;
        l4_entry->allocated = 1;
        l4_entry->present = 1;
        l4_entry->rw = 1;

        //invalidate/reload page directory
        asm ("mov %0, %%cr3" :: "r"(&P4));
    }
    struct PageTable_L3 *l3 = (struct PageTable_L3 *)((*((uint64_t *)l4_entry)) & ~0xFFF);
    
    l3Index = (((uint64_t)virtualPointer) >> 30) & 0x1FF; //bits 30-38
    struct P3_entry *l3_entry = &l3->entries[l3Index];
    if(!l3_entry->present){
        //create frame for l2 table
        void *newPage = MMU_pf_alloc();
        memset1(newPage, 0, PAGE_SIZE);
        *(uint64_t*)(l3_entry) = (uint64_t)newPage;
        l3_entry->present = 1;
        l3_entry->allocated = 1;
        l3_entry->rw = 1;
    }
    if(l3_entry->big_page){
        //1 GB page case
        return;
    }
    struct PageTable_L2 *l2 = (struct PageTable_L2 *)((*((uint64_t *)l3_entry)) & ~0xFFF);


    l2Index = (((uint64_t)virtualPointer) >> 21) & 0x1FF; //bits 21-29
    struct P2_entry *l2_entry = &l2->entries[l2Index];
    if(!l2_entry->present){
        //create frame for l1 table
        void *newPage = MMU_pf_alloc();
        memset1(newPage, 0, PAGE_SIZE);
        *(uint64_t*)(l2_entry) = (uint64_t)newPage;
        l2_entry->present = 1;
        l2_entry->allocated = 1;
        l2_entry->rw = 1;
    }
    if(l2_entry->big_page){
        //2MB page case
        return;
    }
    struct PageTable_L1 *l1 = (struct PageTable_L1 *)((*((uint64_t *)l2_entry)) & ~0xFFF);
    
    l1Index = (((uint64_t)virtualPointer) >> 12) & 0x1FF; //bits 12-20
    struct P1_entry *l1_entry = &l1->entries[l1Index];
    l1_entry->allocated = 1;
    //l1 is set available but the page isn't allocated yet

    //DEBUG
    // if(!l1_entry->present){
    //     //create frame for page
    //     void *newPage = MMU_pf_alloc();
    //     memset1(newPage, 0, PAGE_SIZE);
    //     *(uint64_t*)(l1_entry) = (uint64_t)newPage;
    //     l1_entry->present = 1;
    //     l1_entry->allocated = 1;
    //     l1_entry->rw = 1;
    // }
    // else{
    //     printk("Error: allocating twice!!\n");
    // }
    asm ("mov %0, %%cr3" :: "r"(&P4));
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
    setAllocated(nextVirtualPage, &P4);
    nextVirtualPage += PAGE_SIZE;
    return ret;
}

void *MMU_alloc_pages(int num)
{
    int i;
    void *ret = nextVirtualPage;
    for(i = 0; i < num; i++){
        setAllocated(nextVirtualPage, &P4);
        nextVirtualPage += PAGE_SIZE;
    }
    return ret;
}

void MMU_free_page(void *virtualAddress)
{
    void *physAddr = deallocate(virtualAddress, &P4);
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

    printk("Page fault address: %lx\n", virtualAddr);

    struct PageTable_L1 *l1 = getPageTableL1((void*)virtualAddr, &P4);
    uint64_t l1Index = (virtualAddr >> 12) & 0x1FF; //bits 12-20
    if(l1 == 0){
        printk("error getting page table\n");
        asm ("hlt");
        return;
    }
    struct P1_entry *l1_entry = &l1->entries[l1Index];
    
    if(l1_entry->allocated){
        if(l1_entry->present){
            printk("this shouldn't happen\n");
            asm ("hlt");
        }
        void *backingPage = MMU_pf_alloc();
        memset1(backingPage, 0, PAGE_SIZE);
        *(uint64_t *)(l1_entry) = (uint64_t)backingPage;
        l1_entry->present = 1;
        l1_entry->allocated = 1;
        l1_entry->rw = 1;
        asm ("mov %0, %%cr3" :: "r"(&P4));
    }
    else{
        printPageFaultError(virtualAddr, pageTableAddr, error);
        asm ("hlt");
    }
}
#include "test.h"
#include "paging.h"
#include "vga.h"
#include <stdint.h>

void test_pf_allocator()
{
    int i;
    void * p1, *p2, *p3, *p4;
    printk("Testing page frame allocator\n");
    p1 = MMU_pf_alloc();
    printk("First address is %p\n", p1);
    p2 = MMU_pf_alloc();
    printk("Second address is %p\n", p2);
    p3 = MMU_pf_alloc();
    printk("Third address is %p\n", p3);
    
    printk("Freeing 2, then 1, then 3\n");
    MMU_pf_free(p2);
    MMU_pf_free(p1);
    MMU_pf_free(p3);

    p1 = MMU_pf_alloc();
    printk("Alloc new address, should match p3: %p\n", p1);
    p2 = MMU_pf_alloc();
    printk("Alloc new address, should match p1: %p\n", p2);
    p3 = MMU_pf_alloc();
    printk("Alloc new address, should match p2: %p\n", p3);
    p4 = MMU_pf_alloc();
    printk("Alloc new address, should be unique: %p\n", p4);

    //free all
    MMU_pf_free(p1);
    MMU_pf_free(p2);
    MMU_pf_free(p3);
    MMU_pf_free(p4);

    //allocate all pages at once
    uint64_t pageCount = 0;
    while((p1 = MMU_pf_alloc()) != 0){
        pageCount ++;

        //write bit pattern
        for(i = 0; i < 4096/sizeof(uint64_t); i++){
            uint64_t *ptr = (uint64_t *)p1;
            ptr[i] = (uint64_t)p1;
        }

        //read back bit pattern
        for(i = 0; i < 4096/sizeof(uint64_t); i++){
            uint64_t *ptr = (uint64_t *)p1;
            if(ptr[i] != (uint64_t)p1){
                printk("Error, page %ld bit pattern missmatch\n", pageCount);
                return;
            }
        }
    }

    printk("Allocated %ld pages\n", pageCount);
}
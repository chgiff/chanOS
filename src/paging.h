#ifndef PAGING_H
#define PAGING_H

//allocate physical page frames
extern void *MMU_pf_alloc();
extern void MMU_pf_free(void *pf);

//allocate virtual pages
extern void *MMU_alloc_page();
extern void *MMU_alloc_pages(int num);
extern void MMU_free_page(void *);
extern void MMU_free_pages(void *, int num);

extern void setup_identity();

extern void pageFaultISR(int interrupt, int error, void *data);

#endif
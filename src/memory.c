#include "memory.h"
#include "paging.h"

#define PAGE_SIZE 4096

struct MemBlock{
    void* next;
};

struct MemPool{
    int blockSize;
    int count;
    struct MemBlock *head;
};

struct BlockHeader {
    struct MemPool *pool;
    uint64_t size;
};

struct MemPool pools[6] = {
    {32, 0, 0}, 
    {64, 0, 0}, 
    {128, 0, 0},
    {512, 0, 0},
    {1024, 0, 0},
    {2048, 0, 0}
};

void fillPool(struct MemPool *pool)
{
    int blkSize = pool->blockSize;
    void *page = MMU_alloc_page();
    if(page == 0){
        //error getting new page
        return;
    }

    for(int i = 0; i < PAGE_SIZE/blkSize; i++){
        struct MemBlock *blk = (struct MemBlock *)(page + i*blkSize);
        blk->next = pool->head;
        pool->head = blk;
        pool->count ++; 
    }
}

void *getBlock(struct MemPool *pool, uint64_t size)
{
    if(pool->count == 0){
        //get more memory
        fillPool(pool);
        if(pool->count == 0){
            //failed to get more memory
            return (void*)0;
        }
    }

    struct BlockHeader *header = (struct BlockHeader *)pool->head;
    pool->head = pool->head->next;
    pool->count --;

    header->pool = pool;
    header->size = size;

    return header + 1;
}

void *kmalloc(uint64_t size)
{
    //search for best fit
    for(int i = 0; i < 6; i ++){
        if(size < pools[i].blockSize - sizeof(struct BlockHeader)){
            return getBlock(&pools[i], size);
        }
    }
    
    //allocate whole pages
    void * addr = MMU_alloc_pages((size + sizeof(struct BlockHeader) + PAGE_SIZE - 1)/PAGE_SIZE);
    if(addr == 0){
        //error
        return 0;
    }
    struct BlockHeader *header = (struct BlockHeader *)addr;
    header->pool = 0;
    header->size = size;
    return (void *)(header + 1);
}

void kfree(void *addr)
{
    struct BlockHeader *header = (struct BlockHeader *)(addr - sizeof(struct BlockHeader));

    if(header->pool == 0){
        //special case, free whole pages
        MMU_free_pages((void*)header, (header->size + sizeof(struct BlockHeader) + PAGE_SIZE - 1)/PAGE_SIZE);
        return;
    }

    struct MemPool *pool = header->pool;
    struct MemBlock *freeBlock = (struct MemBlock *)header;

    freeBlock->next = pool->head;
    pool->head = freeBlock;
}

void *memset1(void *s, int c, unsigned int n)
{
    unsigned char *mem = (unsigned char *)s;
    int i;
    for(i = 0; i < n; i ++)
    {
        mem[i] = c;
    }

    return s;
}

void *memmove1(void *dest, const void *src, unsigned int n)
{
    const unsigned char *srcBytes = (unsigned char *)src;
    unsigned char *destBytes = (unsigned char *)dest;
    int i;

    if(dest > src){
        for(i = n-1; i >=0; i--){
            destBytes[i] = srcBytes[i];
        }
    }
    else{
        for(i = 0; i < n; i++){
            destBytes[i] = srcBytes[i];
        }
    }

    return dest;
}

inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

inline void io_wait(void)
{
    /* Port 0x80 is used for 'checkpoints' during POST. */
    /* The Linux kernel seems to think it is free for use :-/ */
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
    /* %%al instead of %0 makes no difference.  TODO: does the register need to be zeroed? */
}



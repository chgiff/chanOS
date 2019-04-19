#include "memory.h"

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
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
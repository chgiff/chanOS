#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

extern void *memset1(void *s, int c, unsigned int n);
extern void *memmove1(void *dest, const void *src, unsigned int n);

extern inline void outb(uint16_t port, uint8_t val);
extern inline uint8_t inb(uint16_t port);
extern inline void io_wait(void);

#endif
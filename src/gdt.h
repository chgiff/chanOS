#ifndef GDT_H
#define GDT_H

#include "stdint.h"

extern void init_gdt();

extern uint64_t getCodeOffset();
extern uint64_t getTSSOffset();
extern uint8_t getTssInterruptStack();
extern uint8_t getTssGPStack();
extern uint8_t getTssDFStack();
extern uint8_t getTssPFStack();

#endif
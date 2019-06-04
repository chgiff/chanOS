#include "blockdev.h "

enum BlockDevType { MASS_STORAGE, PARTITION };

struct BlockDev {
   uint64_t tot_length;
   int (*read_block)(struct BlockDev *dev, uint64_t blk_num, void *dst);
   uint32_t blk_size;
   enum BlockDevType type;
   const char *name;
   uint8_t fs_type;
   struct BlockDev *next;
};

struct ATABlockDev {
   struct BlockDev dev;
   uint16_t ata_base, ata_master;
   uint8_t slave, irq;
   struct ATARequest *req_head, *req_tail;
};

struct ATABlockDev *ata_probe(uint16_t base, uint16_t master,
         uint8_t slave, const char *name, uint8_t irq)
{
       
}


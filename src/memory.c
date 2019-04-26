#include "memory.h"

#define BASIC_MEM_INFO_TAG 4
#define BIOS_BOOT_DEV_TAG 5
#define BOOT_CMD_LINE_TAG 1
#define BOOT_LOADER_NAME_TAG 2
#define MEM_MAP_TAG 6
#define ELF_SYM_TAG 9

//global boot data
char *cmdLine;
char *bootLoaderName;

struct GenericTag {
    uint32_t type;
    uint32_t size;
}__attribute__((packed)); 

struct BasicMemTag {
    uint32_t type; //4
    uint32_t size; //16
    uint32_t lowMem;
    uint32_t highMem;
}__attribute__((packed));

struct BootDevTag {
    uint32_t type; //5
    uint32_t size; //20
    uint32_t bootDev;
    uint32_t bootPart;
    uint32_t bootSubPart;
}__attribute__((packed));

struct BootCmdLineTag {
    uint32_t type; //1
    uint32_t size; 
    //c string (variable)
}__attribute__((packed));

struct BootLoaderNameTag {
    uint32_t type; //2
    uint32_t size; 
    //c string (variable)
}__attribute__((packed));

struct MemMapEntry {
    uint64_t startAddr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
}__attribute__((packed));

struct MemMapTag {
    uint32_t type; //6
    uint32_t size; 
    uint32_t entrySize; //24?
    //list of MemMapEntry
}__attribute__((packed));

struct ElfSectionHeader {
    uint32_t nameIndex;
    uint32_t type;
    uint32_t flags;
    uint64_t address;
    uint64_t segOffset;
    uint64_t segSize;
    uint32_t tableIndex;
    uint32_t extraInfo;
    uint64_t addrAlignment;
    uint64_t entrySize;
}__attribute__((packed));

struct ElfSymTag {
    uint32_t type; //9
    uint32_t size;
    uint32_t numSecEntries;
    uint32_t secEntrySize;
    uint32_t indexStringTable;
    //array of section headers
}__attribute__((packed));

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


void parseBasicMemTag(struct BasicMemTag *tag)
{
    //shouldn't use
}

void parseBootDevTag(struct BootDevTag *tag)
{
    //TODO
}

void parseMemMapEntry(struct MemMapEntry *entry)
{
    //type one is free for use
    if(entry->type == 1){
        //TODO
    }
}

void parseMemMapTag(struct MemMapTag *tag)
{
    int i;
    struct MemMapEntry *curEntry;
    uint32_t entries = (tag->size - sizeof(struct MemMapTag))/tag->entrySize;

    curEntry = (struct MemMapEntry *)(tag + 1);

    for(i = 0; i < entries; i++){
        parseMemMapEntry(curEntry);
        curEntry++;
    }
}

void parseElfSymbols(struct ElfSymTag *tag)
{

}

void parseBootTags(void *tagHeader)
{
    void *curPointer;
    //uint32_t totalSize;
    char parsingTags = 1;

    //totalSize = *(uint32_t *)tagHeader;
    curPointer = tagHeader;
    curPointer += 8; //move to begining of tag entries

    while(parsingTags){
        struct GenericTag *genericTag = (struct GenericTag *)curPointer;

        switch(genericTag->type){
            case BASIC_MEM_INFO_TAG: 
                parseBasicMemTag((struct BasicMemTag*) curPointer);
                break;
            case BIOS_BOOT_DEV_TAG:
                parseBootDevTag((struct BootDevTag*) curPointer);
                break;
            case BOOT_CMD_LINE_TAG:
                cmdLine = curPointer + 8;
                break;
            case BOOT_LOADER_NAME_TAG:
                bootLoaderName = curPointer + 8;
                break;
            case MEM_MAP_TAG:
                parseMemMapTag((struct MemMapTag*) curPointer);
                break;
            case ELF_SYM_TAG:
                parseElfSymbols((struct ElfSymTag*) curPointer);
                break;
            case 0:
                //terminates tag list
                parsingTags = 0;
                break;
            default:
                //TODO error
                break;
        }

        curPointer += genericTag->size;
    }
}
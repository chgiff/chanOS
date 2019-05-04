#include "multiboot.h"
#include <stdint.h>

#define PAGE_SIZE 4096

#define BASIC_MEM_INFO_TAG 4
#define BIOS_BOOT_DEV_TAG 5
#define BOOT_CMD_LINE_TAG 1
#define BOOT_LOADER_NAME_TAG 2
#define MEM_MAP_TAG 6
#define ELF_SYM_TAG 9

//pointer to list of multiboot memmap entries
struct MemMapEntry *memMapEntries;
int memMapEntriesCount;

//pointer to list of ELF headers
struct ElfSectionHeader *elfSectionEntries;
int elfSectionEntriesCount;

//pointer to next invalid section of memory
uint64_t nextInvalid = 0;


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
    uint32_t entrySize; //24
    uint32_t version; //0
    //list of MemMapEntry
}__attribute__((packed));

struct ElfSectionHeader {
    uint32_t nameIndex;
    uint32_t type;
    uint64_t flags;
    uint64_t address;
    uint64_t diskOffset;
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

}

void parseMemMapTag(struct MemMapTag *tag)
{
    memMapEntriesCount = (tag->size - sizeof(struct MemMapTag))/tag->entrySize;
    memMapEntries = (struct MemMapEntry *)(tag + 1);

    while(memMapEntriesCount > 0){
        if(memMapEntries[0].type == 1) break; //mem available for use (type 1)
        memMapEntries = &(memMapEntries[1]);
        memMapEntriesCount --;
    }
}

void parseElfSymbols(struct ElfSymTag *tag)
{
    elfSectionEntriesCount = tag->numSecEntries;
    elfSectionEntries = (struct ElfSectionHeader *)(tag + 1);
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
                //unknown tag
                break;
        }

        //increment by size (also padded to nearest 8 byte alighment)
        curPointer += ((genericTag->size + 7) & ~7);
    }
}

char *getBootLoaderName() { return bootLoaderName;}
char *getCommandLine() { return cmdLine;}

//returns how far to skip to get to a valid page
uint64_t checkRestrictedZones(uint64_t pointer)
{
    int i;
    uint64_t ret = 0;

    if(pointer < nextInvalid) return 0;

    nextInvalid = 0xFFFFFFFFFFFFFFFF;

    for(i = 0; i < elfSectionEntriesCount; i++){
        if(pointer + PAGE_SIZE >= elfSectionEntries[i].address){
            if(pointer < elfSectionEntries[i].address + elfSectionEntries[i].segSize){
                uint64_t diff = elfSectionEntries[i].address + elfSectionEntries[i].segSize - pointer;
                ret += diff;
                pointer += diff;
            }
        }
        else if(elfSectionEntries[i].address < nextInvalid){
            nextInvalid = elfSectionEntries[i].address;
        }
    }

    //special case, protect the multiboot info
    if(pointer + PAGE_SIZE >= (uint64_t)memMapEntries){
        if(pointer < (uint64_t)memMapEntries + (memMapEntriesCount + sizeof(struct MemMapEntry))){
            uint64_t diff = (uint64_t)memMapEntries + (memMapEntriesCount + sizeof(struct MemMapEntry)) - pointer;
            ret += diff;
            pointer += diff;
        }
    }
    else if((uint64_t)memMapEntries < nextInvalid){
        nextInvalid = (uint64_t)memMapEntries;
    }
    if(pointer + PAGE_SIZE >= (uint64_t)elfSectionEntries){
        if(pointer < (uint64_t)elfSectionEntries + (elfSectionEntriesCount + sizeof(struct ElfSectionHeader))){
            uint64_t diff = (uint64_t)elfSectionEntries + (elfSectionEntriesCount + sizeof(struct ElfSectionHeader)) - pointer;
            ret += diff;
            pointer += diff;
        }
    }

    return ret;
}

void *getNewPage()
{
    if(memMapEntriesCount == 0){
        //out of memory
        return (void*)0;
    }
    if(memMapEntries[0].startAddr == 0){
        memMapEntries[0].startAddr += PAGE_SIZE;
        memMapEntries[0].length -= PAGE_SIZE;
    }

    uint64_t skipSize = checkRestrictedZones(memMapEntries[0].startAddr);

    //check if we've exhasted this memMap entry
    if(skipSize + PAGE_SIZE > memMapEntries[0].length){
        while(memMapEntriesCount > 0){
            memMapEntries = &(memMapEntries[1]);
            memMapEntriesCount --;
            //if valid break loop
            if(memMapEntries[0].type == 1) break;
        }
        nextInvalid = 0;
        return getNewPage();
    }

    //update memMap entry
    uint64_t retAddr = (memMapEntries[0].startAddr + skipSize + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
    memMapEntries[0].length -= (retAddr - memMapEntries[0].startAddr + PAGE_SIZE);
    memMapEntries[0].startAddr = retAddr + PAGE_SIZE;
   
    return (void *)retAddr;
}
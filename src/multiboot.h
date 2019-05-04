#ifndef MULTIBOOT_H
#define MULTIBOOT_H

extern void parseBootTags(void *tagHeader);
extern char* getBootLoaderName();
extern char* getCommandLine();
extern void *getNewPage();

#endif
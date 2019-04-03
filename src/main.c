#include "vga.h"
#define WHITE_TXT 0x07
//static unsigned int *vgaBuff = (unsigned int*)0xb8000;

void kmain()
{
    //char loop = 0;
    //while(!loop);
    


    VGA_clear();
   //  unsigned int addr = 0xb8000;
   //  for(int i = 0; i < 80*25; i ++){
    //     //vgaBuff[i] = 0x0720;
    //     //*(vgaBuff + i) = 0x07200720;
    //     //*vgaBuff++ = 0x07; //character
    //     //*vgaBuff++ = 0x20; //color
    //     *((unsigned short*)addr) = 0x0720;
    //     addr += 2;
    //     *((char*)addr) = 0x07;
     //    addr++;
    // }

    // *((int*)0xb8000)=0x07200720;
    // *((int*)0xb8004)=0x07200720;
    // *((int*)0xb8008)=0x07200720;
    // *((int*)0xb80012)=0x07200720;
    

    //vgaBuff[0] = 0x4f524f45;
    //vgaBuff[1] = 0x4f3a4f52;
    //vgaBuff[2] = 0x4f204f20;

    // while(1){
        VGA_display_char('h');
        VGA_display_char('e');
        VGA_display_char('l');
        VGA_display_char('l');
        VGA_display_char('o');
        VGA_display_char(' ');
        VGA_display_char('w');
        VGA_display_char('o');
        VGA_display_char('r');
        VGA_display_char('l');
        VGA_display_char('d');
    // }
}
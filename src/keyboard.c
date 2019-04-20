#include "keyboard.h"
#include "memory.h"
#include "vga.h"

char asccode[58][2] =       /* Array containing ascii codes for
			       appropriate scan codes */
     {
       {   0,0   } ,
       { 0,0 } ,
       { '1','!' } ,
       { '2','@' } ,
       { '3','#' } ,
       { '4','$' } ,
       { '5','%' } ,
       { '6','^' } ,
       { '7','&' } ,
       { '8','*' } ,
       { '9','(' } ,
       { '0',')' } ,
       { '-','_' } ,
       { '=','+' } ,
       {   8,8   } ,
       {   9,9   } ,
       { 'q','Q' } ,
       { 'w','W' } ,
       { 'e','E' } ,
       { 'r','R' } ,
       { 't','T' } ,
       { 'y','Y' } ,
       { 'u','U' } ,
       { 'i','I' } ,
       { 'o','O' } ,
       { 'p','P' } ,
       { '[','{' } ,
       { ']','}' } ,
       {  10,10  } ,
       {   0,0   } ,
       { 'a','A' } ,
       { 's','S' } ,
       { 'd','D' } ,
       { 'f','F' } ,
       { 'g','G' } ,
       { 'h','H' } ,
       { 'j','J' } ,
       { 'k','K' } ,
       { 'l','L' } ,
       { ';',':' } ,
       {  39,34  } ,
       { '`','~' } ,
       {   0,0   } ,
       { '\\','|'} ,
       { 'z','Z' } ,
       { 'x','X' } ,
       { 'c','C' } ,
       { 'v','V' } ,
       { 'b','B' } ,
       { 'n','N' } ,
       { 'm','M' } ,
       { ',','<' } ,
       { '.','>' } ,
       { '/','?' } ,
       {   0,0   } ,
       {   0,0   } ,
       {   0,0   } ,
       { ' ',' ' } ,
   };

//ps/2 ports
#define PS2_DATA_PORT 0x60
#define PS2_COMMAND_PORT 0x64
#define PS2_STATUS_PORT 0x64

#define OUT_BUFF_STATUS 0x01
#define IN_BUF_STATUS 0x02

//ps/2 commands
#define CMD_DISABLE_PS2_PORT_1 0xAD
#define CMD_ENABLE_PS2_PORT_1 0xAE
#define CMD_DISABLE_PS2_PORT_2 0xA7
#define CMD_ENABLE_PS2_PORT_2 0xA8
#define CMD_READ_BYTE_0 0x20
#define CMD_WRITE_BYTE_0 0x60

//ps/2 config bytes
#define FLAG_PS2_PORT_1_INTERRUPT 0x01
#define FLAG_PS2_PORT_2_INTERRUPT 0x02
#define FLAG_PS2_PORT_1_CLOCK 0x10
#define FLAG_PS2_PORT_2_CLOCK 0x20
#define FLAG_PS2_PORT_1_TRANSLATION 0x40

//keyboard commands
#define CMD_RESET_KEYBOARD 0xFF
#define CMD_SET_SCANCODE 0xF0
#define CMD_ENABLE_SCANNING 0xF4
#define RESP_RESEND 0xFE

char getStatus()
{
    return inb(PS2_STATUS_PORT);
}

void sendCommand(char command)
{
    outb(PS2_COMMAND_PORT, command);
}

char readData()
{
    while(!(getStatus() & OUT_BUFF_STATUS)); //wait for data to be ready
    return inb(PS2_DATA_PORT);
}

void writeData(char data)
{
    while(getStatus() & IN_BUF_STATUS); //wait until ready for input
    outb(PS2_DATA_PORT, data);
}

void initializeKeyboard()
{
    /*
    Disable port 1 (see the command table in the previous link)
    Disable port 2
    Read byte 0 from RAM (current configuration)
    Modify the bits in the configuration so the first clock and first interrupt are enabled, but the second clk + interrupt are not.
    Write the configuration byte back to the controller. Note that you will need to poll the status bit to determine when it is safe to write the configuration byte.
    */
    char resp;

    sendCommand(CMD_DISABLE_PS2_PORT_1);
    sendCommand(CMD_DISABLE_PS2_PORT_2);

    sendCommand(CMD_READ_BYTE_0);
    char config = readData();

    config |= (FLAG_PS2_PORT_1_CLOCK | FLAG_PS2_PORT_1_INTERRUPT); //turn on first ps/2 port
    config &= ~(FLAG_PS2_PORT_2_CLOCK | FLAG_PS2_PORT_2_INTERRUPT); //turn off second ps/2 port

    sendCommand(CMD_WRITE_BYTE_0);
    writeData(config);

    sendCommand(CMD_ENABLE_PS2_PORT_1);

    //reset keyboard
    writeData(CMD_RESET_KEYBOARD);
    resp = readData(); //TODO receive response

    //set scan set 1
    writeData(CMD_SET_SCANCODE);
    writeData(0x01);
    resp = readData(); //TODO receive ACK

    //turn on scan codes
    writeData(CMD_ENABLE_SCANNING);
    resp = readData(); //receive ACK
    while(resp == RESP_RESEND)
    {
        writeData(CMD_ENABLE_SCANNING);
        resp = readData();
    }
}


void setKeyboardInterrupts(char on)
{

}

unsigned char getKey()
{
    unsigned int code;
    unsigned char c;
    do{
        while((code = readData()) >= 58);
        c = asccode[code][0];
    }while(!c);
    return c;
}


void keyboardISR(int interrupt, int error, void *data)
{
    unsigned char code;
    code = readData();
    if(code < 58){
        printk("%c", asccode[code][0]);
    }
}
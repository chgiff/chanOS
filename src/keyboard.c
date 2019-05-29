#include "keyboard.h"
#include "memory.h"
#include "multitask.h"
#include "interrupt.h"

unsigned char scancodeMap[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '+', /*'´' */0, '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '<',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '-',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,  '<',
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
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

#define BUFFER_SIZE 128
struct KeyboardState {
    char buffer[BUFFER_SIZE];
    char *head, *tail;
    struct ProcessQueue keyboardQueue;
};
struct KeyboardState state;

char getStatus()
{
    return inb(PS2_STATUS_PORT);
}

void sendCommand(char command)
{
    while(getStatus() & IN_BUF_STATUS); //wait until ready for input
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
    unsigned char resp;

    PROC_init_queue(&state.keyboardQueue);
    state.head = &state.buffer[0];
    state.tail = state.head;

    sendCommand(CMD_DISABLE_PS2_PORT_1);
    sendCommand(CMD_DISABLE_PS2_PORT_2);
    inb(PS2_DATA_PORT); //flush

    sendCommand(CMD_READ_BYTE_0);
    char config = readData();

    config |= (FLAG_PS2_PORT_1_CLOCK | FLAG_PS2_PORT_1_INTERRUPT); //turn on first ps/2 port
    config &= ~(FLAG_PS2_PORT_2_CLOCK | FLAG_PS2_PORT_2_INTERRUPT); //turn off second ps/2 port

    sendCommand(CMD_WRITE_BYTE_0);
    writeData(config);

    sendCommand(CMD_ENABLE_PS2_PORT_1);

    //disable scanning
    inb(PS2_DATA_PORT); //flush
    writeData(0xF5);
    resp = readData();

    //reset keyboard
    writeData(CMD_RESET_KEYBOARD);
    resp = readData(); //TODO receive response
    resp = readData(); //TODO ???

    //set scan set 1
    writeData(CMD_SET_SCANCODE);
    writeData(0x00);
    resp = readData(); //TODO receive ACK
    resp = readData(); //TODO ??
    resp = readData(); //TODO ??

    //turn on scan codes
    writeData(CMD_ENABLE_SCANNING);
    resp = readData(); //receive ACK
    while(resp == RESP_RESEND)
    {
        writeData(CMD_ENABLE_SCANNING);
        resp = readData();
    }
}

unsigned char getKey()
{
    unsigned int code;
    unsigned char c;
    do{
        while((code = readData()) >= 128 && scancodeMap[code]);
        c = scancodeMap[code];
    }while(!c);
    return c;
}


void keyboardISR(int interrupt, int error, void *data)
{
    unsigned char code;
    code = readData();
    if(code < 128 && scancodeMap[code]){
        *state.head = scancodeMap[code];
        state.head ++;
        if(state.head >= state.buffer + BUFFER_SIZE){
            state.head -= BUFFER_SIZE;
        }
        PROC_unblock_all(&state.keyboardQueue);
    }
}

char getChar()
{
    char c;
    CLI;
    while(state.head == state.tail){
        PROC_block_on(&state.keyboardQueue, 1);
        CLI;
    }
    c = *state.tail;
    state.tail ++;
    if(state.tail >= state.buffer + BUFFER_SIZE){
        state.tail -= BUFFER_SIZE;
    }
    STI;
    return c;
}
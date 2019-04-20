#include "serial.h"
#include "memory.h"
#include "interrupt.h"

#define COM_BASE_PORT 0x3F8
#define COM_DATA_PORT (COM_BASE_PORT)
#define COM_DIVISOR_LSB (COM_BASE_PORT)
#define COM_INT_ENABLE_PORT (COM_BASE_PORT + 1)
#define COM_DIVISOR_MSB (COM_BASE_PORT + 1)
#define COM_INT_ID_FIFO_PORT (COM_BASE_PORT + 2)
#define COM_LINE_CONTROL_PORT (COM_BASE_PORT + 3)
#define COM_MODEM_CONTROL_PORT (COM_BASE_PORT + 4)
#define COM_LINE_STATUS_PORT (COM_BASE_PORT + 5)
#define COM_MODEM_STATUS_PORT (COM_BASE_PORT + 6)
#define COM_SCRATCH_PORT (COM_BASE_PORT + 7)

#define TRANSMITTER_EMPTY_INTERRUPT_ENABLE_BIT 0x02
#define TRANSMIT_BUFFER_EMPTY_STATUS_BIT 0x20

#define BUFFER_SIZE 16

struct SerialState {
    char txBuff[BUFFER_SIZE];
    char *txHead, *txTail;

    char hw_tx_busy;
};
struct SerialState state;

void SER_init(void)
{
    outb(COM_INT_ENABLE_PORT, 0x00); //disable interrupts
    outb(COM_LINE_CONTROL_PORT, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM_DIVISOR_LSB, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM_DIVISOR_MSB, 0x00);    //                  (hi byte)
    outb(COM_LINE_CONTROL_PORT, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM_INT_ID_FIFO_PORT, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM_MODEM_CONTROL_PORT, 0x0B);    // IRQs enabled, RTS/DSR set

    state.txHead = state.txTail = &state.txBuff[0];

    state.hw_tx_busy = 0;

    outb(COM_INT_ENABLE_PORT, TRANSMITTER_EMPTY_INTERRUPT_ENABLE_BIT);
}

void SER_hardware_write(const char c)
{
    outb(COM_DATA_PORT, c);
}

//returns number of char written (0 or 1)
int SER_write_char(const char c)
{
    //check if buffer was empty
    if(state.txHead == state.txTail && (inb(COM_LINE_STATUS_PORT) & TRANSMIT_BUFFER_EMPTY_STATUS_BIT)){
        //write immediately to hardware
        SER_hardware_write(c);
        state.hw_tx_busy = 1;
        return 1;
    }
    //else check if buffer is full (tail catches up to head)
    else if(state.txHead - 1 == state.txTail || 
        (state.txHead == &state.txBuff[0] && state.txTail == &state.txBuff[BUFFER_SIZE-1]))
    {
        return 0;    
    }

    *state.txTail = c;
    state.txTail++;

    if(state.txTail >= &state.txBuff[BUFFER_SIZE]){
        state.txTail = &state.txBuff[0];
    }
    return 1;
}

int SER_write_str(const char *buff, int len)
{
    int enableInterrupts, i;
    int bytesWritten = 0;
    enableInterrupts = clearIntConditional();
    for(i = 0; i < len; i++){
        if(!SER_write_char(buff[i])){
            setIntConditional(enableInterrupts);
            return bytesWritten;
        }
        bytesWritten++;
    }
    setIntConditional(enableInterrupts);
    return bytesWritten;
}


void serial_write_isr(int intterupt, int error, void *data)
{
    if(!(inb(COM_LINE_STATUS_PORT) & TRANSMIT_BUFFER_EMPTY_STATUS_BIT)){
        //buffer not empty, ignore
        return;
    }

    //check if there are bytes to consume
    if(state.txHead == state.txTail){
        //no data available, set idle
        state.hw_tx_busy = 0;
        return;
    }

    outb(COM_DATA_PORT, *state.txHead);
    state.txHead++;
    if(state.txHead >= &state.txBuff[BUFFER_SIZE]){
        state.txHead = &state.txBuff[0];
    }
}
#ifndef SERIAL_H
#define SERIAL_H

extern void SER_init(void);
extern int SER_write_str(const char *buff, int len);
extern int SER_write_char(const char c);

extern void serial_write_isr(int intterupt, int error, void *data);

#endif
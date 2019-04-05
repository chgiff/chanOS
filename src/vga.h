#ifndef VGA_H
#define VGA_H

extern void VGA_clear(void);
extern void VGA_display_char(char);
extern void VGA_display_str(const char *);

extern int printk(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#endif
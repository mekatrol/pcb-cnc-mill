#ifndef __DIAGNOSTICS_H__
#define __DIAGNOSTICS_H__


#if DIAG_PRINT_SUPPORTED
void diag_send(unsigned char value);
void diag_flush(void);
void diag_print(const char *message);
void diag_printf(const char *format, ...);
#endif

#endif // __DIAGNOSTICS_H__

#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

void uart_putc(char c);
void uart_puts(const char *s);
int uart_has_data(void);
char uart_getc(void);

#endif

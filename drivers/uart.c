#include <drivers/uart.h>

#define UART0_BASE 0x10000000
#define UART_RBR 0
#define UART_THR 0
#define UART_LSR 5
#define UART_LSR_DATA_READY 0x01

#define UART_REG(offset) ((volatile unsigned char *)(UART0_BASE + (offset)))

/* Write one byte to the QEMU virt 16550-compatible UART. */
void uart_putc(char c)
{
    *UART_REG(UART_THR) = (unsigned char)c;
}

/* Print a NUL-terminated string, translating '\n' to CRLF for terminals. */
void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

/* Nonzero when the UART receive buffer has at least one byte available. */
int uart_has_data(void)
{
    return (*UART_REG(UART_LSR) & UART_LSR_DATA_READY) != 0;
}

/* Busy-wait for one input byte. Interrupt-driven input can replace this later. */
char uart_getc(void)
{
    while (!uart_has_data()) {
        __asm__ volatile("nop");
    }

    return (char)*UART_REG(UART_RBR);
}

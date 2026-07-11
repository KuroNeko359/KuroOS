#include <stdarg.h>
#include <stdint.h>

#include <user/stdio.h>
#include <user/syscall.h>

static unsigned long strlen(const char *s)
{
    unsigned long len = 0;

    while (s[len]) {
        len++;
    }

    return len;
}



void putc(char c)
{
    write(1, &c, 1);
}

void puts(const char *s)
{
    write(1, s, strlen(s));
}

static void print_dec(long value)
{
    char buf[20];
    int i = 0;
    unsigned long x;

    if (value < 0) {
        putc('-');
        x = (unsigned long)(-value);
    } else {
        x = (unsigned long)value;
    }

    if (x == 0) {
        putc('0');
        return;
    }

    while (x > 0) {
        buf[i++] = (char)('0' + (x % 10));
        x /= 10;
    }

    while (i > 0) {
        putc(buf[--i]);
    }
}

static void print_udec(unsigned long x)
{
    char buf[20];
    int i = 0;

    if (x == 0) {
        putc('0');
        return;
    }

    while (x > 0) {
        buf[i++] = (char)('0' + (x % 10));
        x /= 10;
    }

    while (i > 0) {
        putc(buf[--i]);
    }
}

static void print_hex(unsigned long x)
{
    const char *digits = "0123456789abcdef";
    int started = 0;

    for (int i = (int)(sizeof(unsigned long) * 2) - 1; i >= 0; i--) {
        unsigned int nibble = (unsigned int)((x >> (i * 4)) & 0xf);
        if (nibble != 0 || started || i == 0) {
            putc(digits[nibble]);
            started = 1;
        }
    }
}

void printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            putc(*fmt++);
            continue;
        }

        fmt++;
        switch (*fmt) {
        case '\0':
            putc('%');
            va_end(args);
            return;
        case '%':
            putc('%');
            break;
        case 'c':
            putc((char)va_arg(args, int));
            break;
        case 's': {
            const char *s = va_arg(args, const char *);
            puts(s ? s : "(null)");
            break;
        }
        case 'd':
            print_dec((long)va_arg(args, int));
            break;
        case 'u':
            print_udec((unsigned long)va_arg(args, unsigned int));
            break;
        case 'x':
            print_hex((unsigned long)va_arg(args, unsigned int));
            break;
        case 'l':
            fmt++;
            switch (*fmt) {
            case 'd':
                print_dec(va_arg(args, long));
                break;
            case 'u':
                print_udec(va_arg(args, unsigned long));
                break;
            case 'x':
                print_hex(va_arg(args, unsigned long));
                break;
            default:
                putc('%');
                putc('l');
                putc(*fmt);
                break;
            }
            break;
        case 'p':
            puts("0x");
            print_hex((unsigned long)(uintptr_t)va_arg(args, void *));
            break;
        default:
            putc('%');
            putc(*fmt);
            break;
        }

        fmt++;
    }

    va_end(args);
}

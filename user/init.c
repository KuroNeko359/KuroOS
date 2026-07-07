#include <abi/syscall.h>

/* Minimal user-side syscall wrapper for three arguments. */
static long syscall3(long n, long arg0, long arg1, long arg2)
{
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a7 __asm__("a7") = n;

    __asm__ volatile("ecall"
                     : "+r"(a0)
                     : "r"(a1), "r"(a2), "r"(a7)
                     : "memory");

    return a0;
}

/* User-space wrappers. These mirror a tiny subset of POSIX-style calls. */
static long write(int fd, const char *buf, unsigned long len)
{
    return syscall3(SYS_write, fd, (long)buf, len);
}

static long read(int fd, char *buf, unsigned long len)
{
    return syscall3(SYS_read, fd, (long)buf, len);
}

/* exit never returns if the kernel handles it successfully. */
static void exit(int code)
{
    syscall3(SYS_exit, code, 0, 0);

    for (;;) {
    }
}

static unsigned long strlen(const char *s)
{
    unsigned long len = 0;

    while (s[len]) {
        len++;
    }

    return len;
}

static void puts(const char *s)
{
    write(1, s, strlen(s));
}

static int streq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }

    return *a == *b;
}

/* The first user program: a tiny built-in-command shell. */
void user_main(void)
{
    char line[80];

    puts("tiny shell started\n");

    for (;;) {
        puts("> ");

        long n = read(0, line, sizeof(line));
        if (n < 0) {
            puts("read error\n");
            continue;
        }

        if (streq(line, "")) {
            continue;
        }

        if (streq(line, "help")) {
            puts("commands: help hello about exit\n");
        } else if (streq(line, "hello")) {
            puts("hello from user shell\n");
        } else if (streq(line, "about")) {
            puts("riscv-os shell running in U-mode\n");
        } else if (streq(line, "exit")) {
            puts("bye\n");
            exit(0);
        } else {
            puts("unknown command: ");
            puts(line);
            puts("\n");
        }
    }
}

#include <abi/syscall.h>
#include <user/syscall.h>

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

void yield(void)
{
    syscall3(SYS_yield, 0, 0, 0);
}

long write(int fd, const char *buf, unsigned long len)
{
    return syscall3(SYS_write, fd, (long)buf, len);
}

long read(int fd, char *buf, unsigned long len)
{
    return syscall3(SYS_read, fd, (long)buf, len);
}

void exit(int code)
{
    syscall3(SYS_exit, code, 0, 0);

    for (;;) {
    }
}

long getpid(void)
{
    return syscall3(SYS_getpid, 0, 0, 0);
}

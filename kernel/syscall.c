#include <abi/syscall.h>
#include <arch/riscv/csr.h>
#include <drivers/uart.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/sbi.h>
#include <kernel/syscall.h>

/*
 * Write bytes to stdout/stderr. For now both file descriptors are backed by
 * the QEMU virt UART.
 */
static long sys_write(long fd, const char *buf, unsigned long len)
{
    if (fd != 1 && fd != 2) {
        return -1;
    }

    for (unsigned long i = 0; i < len; i++) {
        uart_putc(buf[i]);
    }

    return (long)len;
}

/*
 * Read one line from stdin into a user buffer.
 *
 * This early kernel has no copy_from_user or page tables yet, so user pointers
 * are treated as directly accessible kernel addresses. That is good enough for
 * the first shell, but will need tightening once virtual memory exists.
 */
static long sys_read(long fd, char *buf, unsigned long len)
{
    unsigned long i = 0;

    if (fd != 0) {
        return -1;
    }

    if (len == 0) {
        return 0;
    }

    while (i + 1 < len) {
        char c = uart_getc();

        if (c == '\r') {
            c = '\n';
        }

        if (c == 0x7f || c == '\b') {
            if (i > 0) {
                i--;
                uart_puts("\b \b");
            }
            continue;
        }

        uart_putc(c);

        if (c == '\n') {
            break;
        }

        buf[i++] = c;
    }

    buf[i] = '\0';
    return (long)i;
}

/* Exit the only current user task by asking OpenSBI to power off QEMU. */
static void sys_exit(long code)
{
    printk("\nuser exited with code %d\n", (int)code);
    sbi_shutdown();
    panic("sbi_shutdown returned");
}

/*
 * Dispatch a U-mode syscall.
 *
 * The user ABI is:
 *   a7 = syscall number
 *   a0..a2 = arguments
 *   a0 = return value
 *
 * sepc points at the ecall instruction, so advance it by 4 before returning to
 * user mode. Otherwise sret would execute the same ecall again forever.
 */
void syscall_dispatch(struct trap_frame *tf)
{
    csr_write_sepc(csr_read_sepc() + 4);

    switch (tf->a7) {
    case SYS_write:
        tf->a0 = sys_write((long)tf->a0, (const char *)tf->a1, tf->a2);
        break;
    case SYS_read:
        tf->a0 = sys_read((long)tf->a0, (char *)tf->a1, tf->a2);
        break;
    case SYS_exit:
        sys_exit((long)tf->a0);
        break;
    default:
        printk("unknown syscall: %lu\n", tf->a7);
        panic("unknown syscall");
    }
}

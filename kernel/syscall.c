#include <abi/syscall.h>
#include <drivers/uart.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/sbi.h>
#include <kernel/syscall.h>
#include <kernel/task.h>

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

static struct trap_frame *sys_exit(struct trap_frame *tf, long code)
{
    struct task *task = task_current();
    struct trap_frame *next;

    next = task_exit_current(tf, code);
    printk("\nuser exited with code %d\n", (int)code);

    if (task) {
        printk("task exit: pid=%lu\n", task->pid);
    }

    if (!next) {
        sbi_shutdown();
        panic("sbi_shutdown returned");
    }

    return next;
}

static struct trap_frame *sys_yield(struct trap_frame *tf)
{
    tf->a0 = 0;
    return task_schedule(tf);
}

static long sys_getpid(void)
{
    struct task *task = task_current();

    return task ? (long)task->pid : -1;
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
struct trap_frame *syscall_dispatch(struct trap_frame *tf)
{
    switch (tf->a7) {
    case SYS_write:
        tf->sepc += 4;
        tf->a0 = sys_write((long)tf->a0, (const char *)tf->a1, tf->a2);
        return tf;
    case SYS_read:
        if (!uart_has_data()) {
            return task_schedule(tf);
        }
        tf->sepc += 4;
        tf->a0 = sys_read((long)tf->a0, (char *)tf->a1, tf->a2);
        return tf;
    case SYS_exit:
        tf->sepc += 4;
        return sys_exit(tf, (long)tf->a0);
    case SYS_yield:
        tf->sepc += 4;
        return sys_yield(tf);
    case SYS_getpid:
        tf->sepc += 4;
        tf->a0 = sys_getpid();
        return tf;
    default:
        printk("unknown syscall: %lu\n", tf->a7);
        panic("unknown syscall");
    }

    return tf;
}

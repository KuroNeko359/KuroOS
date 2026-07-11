#include <arch/riscv/csr.h>
#include <kernel/panic.h>
#include <kernel/printk.h>

void panic_at(const char *file, int line, const char *msg)
{
    printk("panic at %s:%d: %s\n", file, line, msg);

    for (;;) {
        cpu_wait_for_interrupt();
    }
}

void panic(const char *msg)
{
    panic_at("?", 0, msg);
}


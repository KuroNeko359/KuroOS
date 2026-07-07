#include <arch/riscv/csr.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/trap.h>
#include <kernel/user.h>

void kernel_main(void)
{
    trap_init();

    printk("Hello, RISC-V OS!\n");
    printk("QEMU virt machine booted this kernel through OpenSBI.\n");
    printk("kernel_main = %p\n", kernel_main);
    printk("printk test: char=%c string=%s dec=%d hex=0x%x percent=%%\n",
           'A', "ok", -12345, 0x2a);
    assert(1 + 1 == 2);

    user_init();
    
    for (;;) {
        cpu_wait_for_interrupt();
    }
}

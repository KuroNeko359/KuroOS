#include <arch/riscv/csr.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/syscall.h>
#include <kernel/trap.h>

extern void trap_entry(void);

/*
 * Point S-mode traps at the assembly entry. RISC-V hardware jumps to stvec
 * for both synchronous exceptions, such as ecall, and interrupts.
 */
void trap_init(void)
{
    csr_write_stvec(trap_entry);
    printk("s-mode trap entry = %p\n", trap_entry);
}

/*
 * Main S-mode trap dispatcher.
 *
 * U-mode ecall uses scause == 8. That is the syscall path. Other traps still
 * stop in panic so failures are visible while the kernel is small.
 */
void trap_handler(struct trap_frame *tf)
{
    unsigned long scause = csr_read_scause();
    unsigned long sepc = csr_read_sepc();
    unsigned long stval = csr_read_stval();
    
    if (scause == 8) {
        syscall_dispatch(tf);
        return;
    }

    printk("\ntrap:\n");
    printk("  scause = 0x%lx\n", scause);
    printk("  sepc   = 0x%lx\n", sepc);
    printk("  stval  = 0x%lx\n", stval);
    printk("  a0     = 0x%lx\n", tf->a0);
    printk("  a7     = 0x%lx\n", tf->a7);
    printk("  sp     = 0x%lx\n", tf->sp);

    panic("unhandled trap");
}

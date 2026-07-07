#ifndef ARCH_RISCV_CSR_H
#define ARCH_RISCV_CSR_H

static inline unsigned long csr_read_scause(void)
{
    unsigned long value;

    __asm__ volatile("csrr %0, scause" : "=r"(value));
    return value;
}

static inline unsigned long csr_read_sepc(void)
{
    unsigned long value;

    __asm__ volatile("csrr %0, sepc" : "=r"(value));
    return value;
}

static inline void csr_write_sepc(unsigned long value)
{
    __asm__ volatile("csrw sepc, %0" : : "r"(value));
}

static inline unsigned long csr_read_stval(void)
{
    unsigned long value;

    __asm__ volatile("csrr %0, stval" : "=r"(value));
    return value;
}

static inline void csr_write_stvec(void *addr)
{
    __asm__ volatile("csrw stvec, %0" : : "r"(addr));
}

static inline void cpu_wait_for_interrupt(void)
{
    __asm__ volatile("wfi");
}

#endif

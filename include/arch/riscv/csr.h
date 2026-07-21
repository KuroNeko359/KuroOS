#ifndef ARCH_RISCV_CSR_H
#define ARCH_RISCV_CSR_H

#define SSTATUS_SIE (1UL << 1)
#define SSTATUS_SPIE (1UL << 5)
#define SSTATUS_SUM (1UL << 18)
#define SIE_STIE    (1UL << 5)
#define SCAUSE_INTERRUPT (1UL << 63)
#define SCAUSE_USER_ECALL 8
#define SCAUSE_SUPERVISOR_TIMER 5

#define SATP_MODE_SV39 (8UL << 60)


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

/**
 * @brief This function will write the value of @param addr into 
 * stvec (Control Status Register), and it is used for configure
 * interrupt/exception entry address in S-model.
 * 
 * @param addr The address of 
 */
static inline void csr_write_stvec(void *addr)
{
    __asm__ volatile("csrw stvec, %0" : : "r"(addr));
}

/**
 * @brief let cpu waitting for interrupt
 */
static inline void cpu_wait_for_interrupt(void)
{
    __asm__ volatile("wfi");
}


static inline unsigned long csr_read_time(void)
{
    unsigned long value;

    __asm__ volatile("csrr %0, time" : "=r"(value));
    return value;
}

static inline unsigned long csr_read_sstatus(void)
{
    unsigned long value;
    __asm__ volatile("csrr %0, sstatus" : "=r"(value));
    return value;
}

static inline void csr_write_sstatus(unsigned long value)
{
    __asm__ volatile("csrw sstatus, %0" : : "r"(value));
}

static inline unsigned long csr_read_sie(void)
{
    unsigned long value;
    __asm__ volatile("csrr %0, sie" : "=r"(value));
    return value;
}

static inline void csr_write_sie(unsigned long value)
{
    __asm__ volatile("csrw sie, %0" : : "r"(value));
}

static inline unsigned long csr_read_satp(void)
{
    unsigned long value;

    __asm__ volatile("csrr %0, satp" : "=r"(value));
    return value;
}

static inline void csr_write_satp(unsigned long value)
{
    __asm__ volatile(
        "csrw satp, %0\n"
        "sfence.vma zero, zero"
        :
        : "r"(value)
        : "memory"
    );
}

#endif

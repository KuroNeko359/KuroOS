#include <arch/riscv/csr.h>
#include <kernel/printk.h>
#include <kernel/sbi.h>
#include <kernel/timer.h>

#define TIMER_INTERVAL 1000000UL

static unsigned long ticks;

static void timer_set_next(void)
{
    sbi_set_timer(csr_read_time() + TIMER_INTERVAL);
}

void timer_init(void)
{
    ticks = 0;
    timer_set_next();
    csr_write_sie(csr_read_sie() | SIE_STIE);
    printk("timer interrupt enabled, interval=%lu\n", TIMER_INTERVAL);
}

void timer_tick(void)
{
    ticks++;
    timer_set_next();
}

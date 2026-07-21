#include <arch/riscv/csr.h>
#include <kernel/initrd.h>
#include <kernel/kmalloc.h>
#include <kernel/list_test.h>
#include <kernel/page.h>
#include <kernel/page_test.h>
#include <kernel/panic.h>
#include <kernel/pgtable.h>
#include <kernel/printk.h>
#include <kernel/trap.h>
#include <kernel/timer.h>
#include <kernel/user.h>
#include <kernel/task.h>
#include <kernel/vm.h>

#include <stdint.h>

#define USER_TEXT_BASE  0x00010000UL
#define USER_STACK_TOP  0x40000000UL

extern char __user_start[];
extern char __user_end[];
extern char _binary_rootfs_cpio_start[];
extern char _binary_rootfs_cpio_end[];
extern void user_main(void);

static void task_create_user_test(void)
{
    struct task *task;
    pte_t *entry_pte;
    pte_t *stack_pte;
    unsigned long entry_offset;

    entry_offset = (unsigned long)user_main - (unsigned long)__user_start;
    task = task_create_user((unsigned long)__user_start,
                            (unsigned long)__user_end,
                            user_main);
    assert(task != 0);
    assert(task->state == TASK_READY);
    assert(task->entry == USER_TEXT_BASE + entry_offset);
    assert(task->user_sp == USER_STACK_TOP);
    assert(task->pagetable != 0);

    entry_pte = pgtable_walk(task->pagetable, task->entry, 0);
    assert(entry_pte != 0);
    assert(pte_present(*entry_pte));
    assert(pte_leaf(*entry_pte));
    assert(pte_flags(*entry_pte) & PTE_U);
    assert(pte_flags(*entry_pte) & PTE_X);

    stack_pte = pgtable_walk(task->pagetable, USER_STACK_TOP - PAGE_SIZE, 0);
    assert(stack_pte != 0);
    assert(pte_present(*stack_pte));
    assert(pte_leaf(*stack_pte));
    assert(pte_flags(*stack_pte) & PTE_U);
    assert(pte_flags(*stack_pte) & PTE_W);

    task_destroy(task);
    printk("task_create_user test passed\n");
}

void kernel_main(void)
{
    unsigned long initrd_start;
    unsigned long initrd_size;

    trap_init();

    printk("Hello, RISC-V OS!\n");
    printk("QEMU virt machine booted this kernel through OpenSBI.\n");
    printk("kernel_main = %p\n", kernel_main);
    printk("printk test: char=%c string=%s dec=%d hex=0x%x percent=%%\n",
           'A', "ok", -12345, 0x2a);
    assert(1 + 1 == 2);
    list_test();
    page_init();
    page_test();
    kmalloc_test();
    pgtable_test();

    vm_init();

    task_create_user_test();

    /* Initialize initrd from embedded data */
    initrd_start = (unsigned long)_binary_rootfs_cpio_start;
    initrd_size = (unsigned long)_binary_rootfs_cpio_end - initrd_start;
    
    printk("initrd: embedded at 0x%lx size=0x%lx\n", initrd_start, initrd_size);
    
    if (initrd_size > 0) {
        initrd_init(initrd_start, initrd_size);
    } else {
        printk("initrd: not found\n");
    }

    timer_init();
    user_init();

    for (;;) {
        cpu_wait_for_interrupt();
    }
}

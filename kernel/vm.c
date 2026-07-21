#include <arch/riscv/csr.h>
#include <kernel/panic.h>
#include <kernel/pgtable.h>
#include <kernel/printk.h>
#include <kernel/vm.h>

/* QEMU virt memory layout. Replace these constants after DTB parsing exists. */
#define UART0_BASE         0x10000000UL
#define QEMU_RAM_BASE      0x80000000UL
#define QEMU_RAM_END       0x88000000UL //128MB

static pagetable_t kernel_pagetable;

void vm_switch(pagetable_t root)
{
    assert(root != 0);
    assert(((unsigned long)root & PAGE_MASK) == 0);

    csr_write_satp(SATP_MODE_SV39 |
                   ((unsigned long)root >> PTE_PAGE_SHIFT));
}

pagetable_t vm_kernel_pagetable(void)
{
    return kernel_pagetable;
}

int vm_map_kernel(pagetable_t root)
{
    assert(root != 0);

    if (pgtable_map(root,
                    QEMU_RAM_BASE,
                    QEMU_RAM_BASE,
                    QEMU_RAM_END - QEMU_RAM_BASE,
                    PAGE_KERNEL) != 0) {
        return -1;
    }

    if (pgtable_map(root,
                    UART0_BASE,
                    UART0_BASE,
                    PAGE_SIZE,
                    PAGE_DEVICE) != 0) {
        return -1;
    }

    return 0;
}

void vm_allow_user_identity(unsigned long addr, unsigned long size)
{
    unsigned long start = addr & ~PAGE_MASK;
    unsigned long end = (addr + size + PAGE_MASK) & ~PAGE_MASK;

    assert(kernel_pagetable != 0);

    for (unsigned long va = start; va < end; va += PAGE_SIZE) {
        pte_t *pte = pgtable_walk(kernel_pagetable, va, 0);

        assert(pte != 0);
        assert(pte_present(*pte));
        assert(pte_leaf(*pte));
        *pte = mk_pte(pte_to_pa(*pte), PAGE_USER | PTE_V);
    }

    __asm__ volatile("sfence.vma zero, zero" ::: "memory");
}

void vm_init(void)
{
    kernel_pagetable = pgtable_create();
    assert(kernel_pagetable != 0);

    assert(vm_map_kernel(kernel_pagetable) == 0);

    vm_switch(kernel_pagetable);

    /* The early syscall path directly accesses user buffers. */
    csr_write_sstatus(csr_read_sstatus() | SSTATUS_SUM);

    printk("vm: Sv39 enabled, satp=0x%lx root=%p\n",
           csr_read_satp(), kernel_pagetable);
}

#include <kernel/page.h>
#include <kernel/panic.h>
#include <kernel/pgtable.h>
#include <kernel/printk.h>

static void page_zero(void *page)
{
    unsigned long *p = page;

    for (unsigned long i = 0; i < PAGE_SIZE / sizeof(*p); i++) {
        p[i] = 0;
    }
}

static unsigned long align_down(unsigned long value, unsigned long align)
{
    return value & ~(align - 1);
}

static unsigned long align_up(unsigned long value, unsigned long align)
{
    return (value + align - 1) & ~(align - 1);
}

pagetable_t pgtable_create(void)
{
    pagetable_t root = page_alloc();

    if (!root) {
        return 0;
    }

    page_zero(root);
    return root;
}

pte_t *pgtable_walk(pagetable_t root, unsigned long va, int alloc)
{
    pagetable_t table = root;

    assert(root != 0);
    assert(va < MAXVA);

    for (int level = SV39_LEVELS - 1; level > 0; level--) {
        pte_t *pte = &table[vpn_index(level, va)];

        if (pte_present(*pte)) {
            assert(!pte_leaf(*pte));
            table = (pagetable_t)pte_to_pa(*pte);
            continue;
        }

        if (!alloc) {
            return 0;
        }

        table = pgtable_create();
        if (!table) {
            return 0;
        }

        *pte = mk_pte((unsigned long)table, PTE_V);
    }

    return &table[vpn_index(0, va)];
}


/**
 * @brief This function maps a contiguous range of virtual addresses to physical addresses in the page table.
 *        It iterates through the specified size, creating page table entries (PTEs) for each page.
 * 
 * @param root  The root page table address (page table directory base).
 * @param va    The starting virtual address to be mapped.
 * @param pa    The starting physical address to map to (must be page-aligned).
 * @param size  The size of the memory region to map, in bytes.
 * @param flags The page table entry flags (e.g., permissions like Read/Write, User/Supervisor).
 * @return int  0 on success, -1 on failure (e.g., if page table walk fails or the virtual page is already mapped).
 */
int pgtable_map(pagetable_t root, unsigned long va, unsigned long pa,
                unsigned long size, unsigned long flags)
{
    unsigned long start = align_down(va, PAGE_SIZE);
    unsigned long end = align_up(va + size, PAGE_SIZE);

    assert(root != 0);
    assert(size != 0);
    assert((pa & PAGE_MASK) == 0);

    for (unsigned long addr = start; addr < end; addr += PAGE_SIZE, pa += PAGE_SIZE) {
        pte_t *pte = pgtable_walk(root, addr, 1);

        if (!pte) {
            return -1;
        }

        if (pte_present(*pte)) {
            return -1;
        }

        *pte = mk_pte(pa, flags | PTE_V);
    }

    return 0;
}

void pgtable_unmap(pagetable_t root, unsigned long va, unsigned long size)
{
    unsigned long start = align_down(va, PAGE_SIZE);
    unsigned long end = align_up(va + size, PAGE_SIZE);

    assert(root != 0);
    assert(size != 0);

    for (unsigned long addr = start; addr < end; addr += PAGE_SIZE) {
        pte_t *pte = pgtable_walk(root, addr, 0);

        assert(pte != 0);
        assert(pte_present(*pte));
        assert(pte_leaf(*pte));

        *pte = __pte(0);
    }
}

void pgtable_free_walk(pagetable_t root)
{
    assert(root != 0);

    for (unsigned long i = 0; i < PTRS_PER_PT; i++) {
        pte_t pte = root[i];

        if (!pte_present(pte)) {
            continue;
        }

        if (pte_leaf(pte)) {
            root[i] = __pte(0);
            continue;
        }

        pgtable_free_walk((pagetable_t)pte_to_pa(pte));
        root[i] = __pte(0);
    }

    page_free(root);
}

void pgtable_test(void)
{
    pagetable_t root = pgtable_create();
    void *page = page_alloc();
    pte_t *pte;
    unsigned long va = 0x400000UL;

    assert(root != 0);
    assert(page != 0);
    assert(pgtable_map(root, va, (unsigned long)page, PAGE_SIZE, PAGE_USER) == 0);

    pte = pgtable_walk(root, va, 0);
    assert(pte != 0);
    assert(pte_present(*pte));
    assert(pte_leaf(*pte));
    assert(pte_to_pa(*pte) == (unsigned long)page);
    assert(pte_flags(*pte) & PTE_U);

    pgtable_unmap(root, va, PAGE_SIZE);
    assert(pte_none(*pgtable_walk(root, va, 0)));

    page_free(page);
    pgtable_free_walk(root);

    printk("pgtable test passed\n");
}

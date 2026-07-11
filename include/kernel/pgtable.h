#ifndef KERNEL_PGTABLE_H
#define KERNEL_PGTABLE_H

#include <kernel/page.h>

typedef unsigned long pteval_t; 

typedef struct { 
    pteval_t pte; 
} pte_t;

typedef pte_t pgd_t;
typedef pte_t pmd_t;
typedef pte_t *pagetable_t;

#define PTRS_PER_PT 512UL
#define SV39_LEVELS 3

#define PTE_V       (1UL << 0)
#define PTE_R       (1UL << 1)
#define PTE_W       (1UL << 2)
#define PTE_X       (1UL << 3)
#define PTE_U       (1UL << 4)
#define PTE_G       (1UL << 5)
#define PTE_A       (1UL << 6)
#define PTE_D       (1UL << 7)

#define PTE_PPN_SHIFT   10
#define PTE_PAGE_SHIFT  12
#define PTE_FLAGS_MASK  ((1UL << PTE_PPN_SHIFT) - 1)
#define PTE_PPN_MASK    (((1UL << 44) - 1) << PTE_PPN_SHIFT)

#define pte_val(x)          ((x).pte)
#define __pte(x)            ((pte_t){ (pteval_t)(x) })
#define pte_none(x)         (pte_val(x) == 0)
#define pte_present(x)      (pte_val(x) & PTE_V)
#define pte_flags(x)        (pte_val(x) & PTE_FLAGS_MASK)
#define pte_pfn(x)          ((pte_val(x) & PTE_PPN_MASK) >> PTE_PPN_SHIFT)
#define pfn_pte(pfn, flags) __pte((((pteval_t)(pfn)) << PTE_PPN_SHIFT) | (pteval_t)(flags))
#define pa_to_pte(pa)       ((((pteval_t)(pa)) >> PTE_PAGE_SHIFT) << PTE_PPN_SHIFT)
#define pte_to_pa(x)        (pte_pfn(x) << PTE_PAGE_SHIFT)
#define mk_pte(pa, flags)   __pte(pa_to_pte(pa) | (pteval_t)(flags))
#define pte_leaf(x)         (pte_val(x) & (PTE_R | PTE_W | PTE_X))

#define PGDIR_SHIFT 30
#define PMD_SHIFT   21
#define PTE_SHIFT   12
#define PGDIR_SIZE  (1UL << PGDIR_SHIFT)
#define PMD_SIZE    (1UL << PMD_SHIFT)

#define VA_BITS 39
#define MAXVA   (1UL << (VA_BITS - 1))

#define vpn_index(level, va) ((((unsigned long)(va)) >> (PTE_SHIFT + 9 * (level))) & 0x1ff)

#define PAGE_KERNEL (PTE_R | PTE_W | PTE_X | PTE_A | PTE_D)
#define PAGE_DEVICE (PTE_R | PTE_W | PTE_A | PTE_D)
#define PAGE_USER   (PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D)

pagetable_t pgtable_create(void);
pte_t *pgtable_walk(pagetable_t root, unsigned long va, int alloc);
int pgtable_map(pagetable_t root, unsigned long va, unsigned long pa,
                unsigned long size, unsigned long flags);
void pgtable_unmap(pagetable_t root, unsigned long va, unsigned long size);
void pgtable_free_walk(pagetable_t root);

void pgtable_test(void);



#endif

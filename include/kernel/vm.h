#ifndef KERNEL_VM_H
#define KERNEL_VM_H

#include <kernel/pgtable.h>

void vm_init(void);
void vm_switch(pagetable_t root);
int vm_map_kernel(pagetable_t root);
void vm_allow_user_identity(unsigned long addr, unsigned long size);
pagetable_t vm_kernel_pagetable(void);

#endif

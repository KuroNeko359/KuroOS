#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <kernel/trap.h>

void syscall_dispatch(struct trap_frame *tf);

#endif

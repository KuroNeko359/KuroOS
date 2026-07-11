#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <kernel/trap.h>

struct trap_frame *syscall_dispatch(struct trap_frame *tf);

#endif

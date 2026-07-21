#ifndef KERNEL_EXEC_H
#define KERNEL_EXEC_H

struct trap_frame;

/*
 * exec() system call implementation.
 * Loads a program from initrd and replaces current process.
 */
long sys_exec(const char *filename, struct trap_frame *tf);

#endif

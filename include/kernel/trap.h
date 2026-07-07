#ifndef KERNEL_TRAP_H
#define KERNEL_TRAP_H

/*
 * Saved user register state.
 *
 * The field order must match arch/riscv/trap.S exactly. trap_entry builds this
 * frame on the kernel stack, passes its address to trap_handler(), then restores
 * the same registers before sret returns to user mode.
 */
struct trap_frame {
    unsigned long ra;
    unsigned long gp;
    unsigned long tp;
    unsigned long t0;
    unsigned long t1;
    unsigned long t2;
    unsigned long s0;
    unsigned long s1;
    unsigned long a0;
    unsigned long a1;
    unsigned long a2;
    unsigned long a3;
    unsigned long a4;
    unsigned long a5;
    unsigned long a6;
    unsigned long a7;
    unsigned long s2;
    unsigned long s3;
    unsigned long s4;
    unsigned long s5;
    unsigned long s6;
    unsigned long s7;
    unsigned long s8;
    unsigned long s9;
    unsigned long s10;
    unsigned long s11;
    unsigned long t3;
    unsigned long t4;
    unsigned long t5;
    unsigned long t6;
    unsigned long sp;
};

/* Install the S-mode trap vector. */
void trap_init(void);

/* Handle traps after trap_entry has saved the interrupted user context. */
void trap_handler(struct trap_frame *tf);

#endif

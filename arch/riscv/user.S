    .section .text
    .globl enter_user
    .align 2

enter_user:
    # a0 = user stack top, a1 = user entry point, a2 = kernel stack top.
    # Keep this task's kernel stack in sscratch so trap_entry can switch back
    # to it when the user program executes ecall or receives an interrupt.
    csrw sscratch, a2
    mv sp, a0
    csrw sepc, a1

    csrr t0, sstatus

    # Clear SPP so sret returns to U-mode, not S-mode.
    li t1, 1
    slli t1, t1, 8
    not t1, t1
    and t0, t0, t1

    # Set SPIE so interrupts are enabled after returning to user mode.
    li t1, 1
    slli t1, t1, 5
    or t0, t0, t1

    csrw sstatus, t0
    sret

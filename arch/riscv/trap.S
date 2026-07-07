    .section .text
    .globl trap_entry
    .align 2

trap_entry:
    # Swap from the interrupted user stack to the saved kernel stack.
    # enter_user() keeps the kernel stack pointer in sscratch.
    csrrw sp, sscratch, sp

    # Build struct trap_frame on the kernel stack. Keep these offsets in sync
    # with include/kernel/trap.h.
    addi sp, sp, -248

    sd ra, 0(sp)
    sd gp, 8(sp)
    sd tp, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd s0, 48(sp)
    sd s1, 56(sp)
    sd a0, 64(sp)
    sd a1, 72(sp)
    sd a2, 80(sp)
    sd a3, 88(sp)
    sd a4, 96(sp)
    sd a5, 104(sp)
    sd a6, 112(sp)
    sd a7, 120(sp)
    sd s2, 128(sp)
    sd s3, 136(sp)
    sd s4, 144(sp)
    sd s5, 152(sp)
    sd s6, 160(sp)
    sd s7, 168(sp)
    sd s8, 176(sp)
    sd s9, 184(sp)
    sd s10, 192(sp)
    sd s11, 200(sp)
    sd t3, 208(sp)
    sd t4, 216(sp)
    sd t5, 224(sp)
    sd t6, 232(sp)

    # After csrrw, sscratch contains the interrupted user sp. Save it in the
    # last trap_frame slot so sret can resume with the original user stack.
    csrr t0, sscratch
    sd t0, 240(sp)

    # C calling convention: a0 is the first argument, so pass trap_frame *.
    mv a0, sp
    call trap_handler

    # Prepare sscratch for the next user trap: it must hold the current kernel
    # stack top before we restore user sp.
    addi t0, sp, 248
    csrw sscratch, t0

    # Restore the user register file from trap_frame.
    ld ra, 0(sp)
    ld gp, 8(sp)
    ld tp, 16(sp)
    ld t0, 24(sp)
    ld t1, 32(sp)
    ld t2, 40(sp)
    ld s0, 48(sp)
    ld s1, 56(sp)
    ld a0, 64(sp)
    ld a1, 72(sp)
    ld a2, 80(sp)
    ld a3, 88(sp)
    ld a4, 96(sp)
    ld a5, 104(sp)
    ld a6, 112(sp)
    ld a7, 120(sp)
    ld s2, 128(sp)
    ld s3, 136(sp)
    ld s4, 144(sp)
    ld s5, 152(sp)
    ld s6, 160(sp)
    ld s7, 168(sp)
    ld s8, 176(sp)
    ld s9, 184(sp)
    ld s10, 192(sp)
    ld s11, 200(sp)
    ld t3, 208(sp)
    ld t4, 216(sp)
    ld t5, 224(sp)
    ld t6, 232(sp)

    ld sp, 240(sp)
    sret

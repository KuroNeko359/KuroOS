    .section .text.entry
    .globl _start

_start:
    la sp, __stack_top

    la t0, __bss_start
    la t1, __bss_end
clear_bss:
    bgeu t0, t1, call_kernel
    sd zero, 0(t0)
    addi t0, t0, 8
    j clear_bss

call_kernel:
    call kernel_main

hang:
    wfi
    j hang


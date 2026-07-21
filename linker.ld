OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x80200000;

  .text : {
    *(.text.entry)
    *(EXCLUDE_FILE (build/user/*.o) .text .text.*)
  }

  . = ALIGN(4096);
  __user_start = .;
  .user : {
    build/user/*.o(.text .text.*)
    build/user/*.o(.srodata .srodata.*)
    build/user/*.o(.rodata .rodata.*)
    build/user/*.o(.sdata .sdata.*)
    build/user/*.o(.data .data.*)
    build/user/*.o(.sbss .sbss.*)
    build/user/*.o(.bss .bss.*)
    build/user/*.o(COMMON)
  }
  . = ALIGN(4096);
  __user_end = .;

  .rodata : {
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
  }

  .data : {
    *(.sdata .sdata.*)
    *(.data .data.*)
  }

  .bss : {
    __bss_start = .;
    *(EXCLUDE_FILE (build/user/*.o) .sbss .sbss.*)
    *(EXCLUDE_FILE (build/user/*.o) .bss .bss.*)
    *(.scommon)
    *(COMMON)
    __bss_end = .;
  }

  . = ALIGN(16);
  . += 4096;
  __stack_top = .;

  . = ALIGN(4096);
  __kernel_end = .;
}

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x80200000;

  .text : {
    *(.text.entry)
    *(.text .text.*)
  }

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
    *(.sbss .sbss.*)
    *(.bss .bss.*)
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

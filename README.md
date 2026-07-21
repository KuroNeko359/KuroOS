# Minimal RISC-V OS

This project has been rewritten in Rust,
https://github.com/KuroNeko359/NekOS

Build:

```sh
make
```

Run:

```sh
make run
```

Exit QEMU:

```text
Ctrl-A, then X
```

Debug:

```sh
make debug
```

In another terminal:

```sh
riscv64-elf-gdb build/kernel.elf
target remote :1234
```

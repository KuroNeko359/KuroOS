TOOLCHAIN ?= riscv64-elf
CC := $(TOOLCHAIN)-gcc
OBJCOPY := $(TOOLCHAIN)-objcopy

CFLAGS := -Wall -Wextra -O2 -g -ffreestanding -nostdlib -Iinclude
CFLAGS += -march=rv64gc -mabi=lp64 -mcmodel=medany
LDFLAGS := -T linker.ld -nostdlib

BUILD := build
KERNEL := $(BUILD)/kernel.elf
ASM_SRCS := arch/riscv/start.S arch/riscv/trap.S arch/riscv/user.S
C_SRCS := drivers/uart.c kernel/main.c kernel/panic.c kernel/printk.c kernel/sbi.c kernel/syscall.c kernel/trap.c kernel/user.c user/init.c
OBJS := $(patsubst %.S,$(BUILD)/%.o,$(ASM_SRCS))
OBJS += $(patsubst %.c,$(BUILD)/%.o,$(C_SRCS))

.PHONY: all run debug clean

all: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.S | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

run: $(KERNEL)
	qemu-system-riscv64 -machine virt -nographic -kernel $(KERNEL)

debug: $(KERNEL)
	qemu-system-riscv64 -machine virt -nographic -kernel $(KERNEL) -S -s

clean:
	rm -rf $(BUILD)

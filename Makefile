TOOLCHAIN ?= riscv64-elf
CC := $(TOOLCHAIN)-gcc
OBJCOPY := $(TOOLCHAIN)-objcopy

CFLAGS := -Wall -Wextra -O2 -g -ffreestanding -nostdlib -Iinclude
CFLAGS += -march=rv64gc -mabi=lp64 -mcmodel=medany
LDFLAGS := -T linker.ld -nostdlib

BUILD := build
KERNEL := $(BUILD)/kernel.elf
ASM_SRCS := arch/riscv/start.S arch/riscv/trap.S arch/riscv/user.S
C_SRCS := drivers/uart.c kernel/kmalloc.c kernel/list_test.c kernel/main.c kernel/page.c kernel/page_test.c kernel/panic.c kernel/pgtable.c kernel/printk.c kernel/sbi.c kernel/syscall.c kernel/task.c kernel/timer.c kernel/trap.c kernel/user.c kernel/vm.c kernel/initrd.c kernel/exec.c user/init.c user/printf.c user/syscall.c
OBJS := $(patsubst %.S,$(BUILD)/%.o,$(ASM_SRCS))
OBJS += $(patsubst %.c,$(BUILD)/%.o,$(C_SRCS))

# User programs
PROGRAMS_DIR := programs
PROGRAMS_BUILD := $(BUILD)/programs
PROGRAMS := hello counter loop
PROGRAM_ELFS := $(patsubst %,$(PROGRAMS_BUILD)/%,$(PROGRAMS))
USER_CFLAGS := -Wall -Wextra -O2 -g -ffreestanding -nostdlib -Iinclude
USER_CFLAGS += -march=rv64gc -mabi=lp64 -mcmodel=medany

# Initrd
INITRD := $(BUILD)/rootfs.cpio
INITRD_OBJ := $(BUILD)/initrd.o

.PHONY: all run debug clean programs

all: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.S | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# User programs
$(PROGRAMS_BUILD):
	mkdir -p $(PROGRAMS_BUILD)

$(PROGRAMS_BUILD)/start.o: $(PROGRAMS_DIR)/start.S | $(PROGRAMS_BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(PROGRAMS_BUILD)/%.o: $(PROGRAMS_DIR)/%.c | $(PROGRAMS_BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(PROGRAMS_BUILD)/printf.o: user/printf.c | $(PROGRAMS_BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(PROGRAMS_BUILD)/syscall.o: user/syscall.c | $(PROGRAMS_BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(PROGRAMS_BUILD)/%: $(PROGRAMS_BUILD)/start.o $(PROGRAMS_BUILD)/%.o $(PROGRAMS_BUILD)/printf.o $(PROGRAMS_BUILD)/syscall.o $(PROGRAMS_DIR)/user.ld
	$(CC) $(USER_CFLAGS) -T $(PROGRAMS_DIR)/user.ld $< $(PROGRAMS_BUILD)/$*.o $(PROGRAMS_BUILD)/printf.o $(PROGRAMS_BUILD)/syscall.o -o $@

# Create initrd cpio archive
$(INITRD): $(PROGRAM_ELFS)
	@echo "Creating initrd..."
	@rm -f $(INITRD)
	@cd $(PROGRAMS_BUILD) && (echo "hello"; echo "counter"; echo "loop") | cpio -o -H newc > ../../$(INITRD)
	@echo "Created $(INITRD)"

# Create initrd object file from cpio archive
$(INITRD_OBJ): $(INITRD)
	@echo "Creating initrd object..."
	@cd $(BUILD) && $(TOOLCHAIN)-ld -r -b binary -o initrd.o rootfs.cpio
	@echo "Created $(INITRD_OBJ)"

$(KERNEL): $(OBJS) $(INITRD_OBJ) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(INITRD_OBJ) -o $@

run: $(KERNEL)
	qemu-system-riscv64 -machine virt -nographic -kernel $(KERNEL)

debug: $(KERNEL)
	qemu-system-riscv64 -machine virt -nographic -kernel $(KERNEL) -S -s

clean:
	rm -rf $(BUILD)

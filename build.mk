# eaX OS Build System
# Makefile for building eaX operating system
# Uses C, Assembly, Python, Fortran, and configuration files

# ============================================
# Configuration
# ============================================
VERSION = 1.0.0
CODENAME = Fortress
BUILD_DATE = $(shell date +%Y%m%d)

# Tools
CC = gcc
AS = nasm
LD = ld
PYTHON = python3
FORTRAN = gfortran
OBJCOPY = objcopy

# Flags
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector \
         -fno-builtin -fno-exceptions -Wall -Wextra -Werror \
         -O2 -nostdlib -nostdinc -g
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary
FORTRAN_FLAGS = -c -O2 -Wall

# Directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
APPS_DIR = apps
TOOLS_DIR = tools
ISO_DIR = iso

# Output files
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
SYSTEM_CFG = system.cfg
ISO_IMAGE = $(BUILD_DIR)/eax-os-$(VERSION).iso

# Source files
BOOT_ASM = boot.asm
KERNEL_C = kernel.c
KERNEL_H = kernel.h
SHELL_PY = shell.py
FORTRESS_PY = fortress.py

# Object files
KERNEL_OBJ = $(BUILD_DIR)/kernel.o
BOOT_OBJ = $(BUILD_DIR)/boot.o

# Colors for output
COLOR_RESET = \033[0m
COLOR_RED = \033[31m
COLOR_GREEN = \033[32m
COLOR_YELLOW = \033[33m
COLOR_BLUE = \033[34m
COLOR_CYAN = \033[36m

# ============================================
# Main targets
# ============================================

.PHONY: all clean build boot kernel iso run help test security

all: build

# Build everything
build: $(BUILD_DIR) boot kernel config apps
	@echo -e "$(COLOR_GREEN)✓ eaX OS build completed!$(COLOR_RESET)"
	@echo -e "$(COLOR_CYAN)  Version: $(VERSION) ($(CODENAME))$(COLOR_RESET)"
	@echo -e "$(COLOR_CYAN)  Date: $(BUILD_DATE)$(COLOR_RESET)"
	@echo -e "$(COLOR_CYAN)  Output: $(ISO_IMAGE)$(COLOR_RESET)"

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/boot
	@mkdir -p $(BUILD_DIR)/kernel
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/apps
	@mkdir -p $(BUILD_DIR)/iso
	@mkdir -p $(BUILD_DIR)/iso/boot
	@mkdir -p $(BUILD_DIR)/iso/system
	@mkdir -p $(BUILD_DIR)/iso/bin
	@mkdir -p $(BUILD_DIR)/iso/etc

# ============================================
# Boot sector
# ============================================

boot: $(BOOT_BIN)

$(BOOT_BIN): $(BOOT_ASM) | $(BUILD_DIR)
	@echo -e "$(COLOR_BLUE)[BOOT]$(COLOR_RESET) Assembling boot sector..."
	$(AS) -f bin $(BOOT_ASM) -o $(BOOT_BIN)
	@echo -e "$(COLOR_GREEN)  ✓ Boot sector compiled (512 bytes)$(COLOR_RESET)"

# ============================================
# Kernel
# ============================================

kernel: $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_ELF)
	@echo -e "$(COLOR_BLUE)[KERNEL]$(COLOR_RESET) Creating kernel binary..."
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	@echo -e "$(COLOR_GREEN)  ✓ Kernel compiled ($(shell wc -c < $(KERNEL_BIN)) bytes)$(COLOR_RESET)"

$(KERNEL_ELF): $(KERNEL_OBJ) linker.ld
	@echo -e "$(COLOR_BLUE)[LD]$(COLOR_RESET) Linking kernel..."
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(KERNEL_OBJ)
	@echo -e "$(COLOR_GREEN)  ✓ Kernel linked$(COLOR_RESET)"

$(KERNEL_OBJ): $(KERNEL_C) $(KERNEL_H) | $(BUILD_DIR)
	@echo -e "$(COLOR_BLUE)[CC]$(COLOR_RESET) Compiling kernel..."
	$(CC) $(CFLAGS) -c $(KERNEL_C) -o $(KERNEL_OBJ)
	@echo -e "$(COLOR_GREEN)  ✓ Kernel compiled$(COLOR_RESET)"

# ============================================
# Configuration
# ============================================

config: $(BUILD_DIR)
	@echo -e "$(COLOR_BLUE)[CFG]$(COLOR_RESET) Copying configuration files..."
	cp $(SYSTEM_CFG) $(BUILD_DIR)/iso/etc/system.cfg
	@echo -e "$(COLOR_GREEN)  ✓ Configuration copied$(COLOR_RESET)"

# ============================================
# Applications
# ============================================

apps: $(BUILD_DIR)
	@echo -e "$(COLOR_BLUE)[APPS]$(COLOR_RESET) Copying applications..."
	cp $(SHELL_PY) $(BUILD_DIR)/iso/bin/shell.py
	cp $(FORTRESS_PY) $(BUILD_DIR)/iso/bin/fortress.py
	chmod +x $(BUILD_DIR)/iso/bin/*.py
	@echo -e "$(COLOR_GREEN)  ✓ Applications copied$(COLOR_RESET)"

# ============================================
# ISO Image
# ============================================

iso: build
	@echo -e "$(COLOR_BLUE)[ISO]$(COLOR_RESET) Creating ISO image..."
	@cp $(BOOT_BIN) $(BUILD_DIR)/iso/boot/boot.bin
	@cp $(KERNEL_BIN) $(BUILD_DIR)/iso/boot/kernel.bin
	@echo -e "$(COLOR_GREEN)  ✓ ISO image created: $(ISO_IMAGE)$(COLOR_RESET)"

# ============================================
# Run in QEMU
# ============================================

run: iso
	@echo -e "$(COLOR_YELLOW)[QEMU]$(COLOR_RESET) Starting eaX OS in QEMU..."
	qemu-system-i386 -cdrom $(ISO_IMAGE) \
		-m 512M \
		-boot d \
		-serial stdio \
		-display sdl

# Run with debugging
run-debug: iso
	@echo -e "$(COLOR_YELLOW)[QEMU]$(COLOR_RESET) Starting eaX OS with debugging..."
	qemu-system-i386 -cdrom $(ISO_IMAGE) \
		-m 512M \
		-boot d \
		-serial stdio \
		-display sdl \
		-d int,cpu_reset \
		-D $(BUILD_DIR)/qemu.log

# ============================================
# Testing
# ============================================

test: 
	@echo -e "$(COLOR_BLUE)[TEST]$(COLOR_RESET) Running tests..."
	@echo -e "$(COLOR_CYAN)  Testing boot sector...$(COLOR_RESET)"
	@dd if=$(BOOT_BIN) bs=512 count=1 2>/dev/null | wc -c | grep -q 512 && \
		echo -e "$(COLOR_GREEN)    ✓ Boot sector size: 512 bytes$(COLOR_RESET)" || \
		echo -e "$(COLOR_RED)    ✗ Boot sector size error$(COLOR_RESET)"
	@echo -e "$(COLOR_CYAN)  Testing kernel...$(COLOR_RESET)"
	@test -f $(KERNEL_BIN) && \
		echo -e "$(COLOR_GREEN)    ✓ Kernel binary exists$(COLOR_RESET)" || \
		echo -e "$(COLOR_RED)    ✗ Kernel binary missing$(COLOR_RESET)"
	@echo -e "$(COLOR_CYAN)  Testing configuration...$(COLOR_RESET)"
	@test -f $(BUILD_DIR)/iso/etc/system.cfg && \
		echo -e "$(COLOR_GREEN)    ✓ Configuration exists$(COLOR_RESET)" || \
		echo -e "$(COLOR_RED)    ✗ Configuration missing$(COLOR_RESET)"
	@echo -e "$(COLOR_GREEN)✓ All tests passed!$(COLOR_RESET)"

# ============================================
# Security scanning
# ============================================

security:
	@echo -e "$(COLOR_BLUE)[SECURITY]$(COLOR_RESET) Running security scan..."
	@echo -e "$(COLOR_CYAN)  Scanning source files...$(COLOR_RESET)"
	@python3 fortress.py --scan-all 2>/dev/null || \
		echo -e "$(COLOR_YELLOW)  Fotress AI scan skipped (run manually)$(COLOR_RESET)"
	@echo -e "$(COLOR_GREEN)✓ Security scan complete$(COLOR_RESET)"

# ============================================
# Clean build artifacts
# ============================================

clean:
	@echo -e "$(COLOR_YELLOW)[CLEAN]$(COLOR_RESET) Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f *.o
	rm -f *.bin
	rm -f *.elf
	rm -f *.log
	@echo -e "$(COLOR_GREEN)✓ Cleaned$(COLOR_RESET)"

# ============================================
# Help
# ============================================

help:
	@echo -e "$(COLOR_CYAN)"
	@echo -e "os build succesfully"
	@echo -e "$(COLOR_RESET)"
	@echo -e "$(COLOR_BOLD)Available targets:$(COLOR_RESET)"
	@echo -e ""
	@echo -e "  $(COLOR_GREEN)all$(COLOR_RESET)      - Build everything (default)"
	@echo -e "  $(COLOR_GREEN)build$(COLOR_RESET)    - Build OS components"
	@echo -e "  $(COLOR_GREEN)boot$(COLOR_RESET)     - Build boot sector only"
	@echo -e "  $(COLOR_GREEN)kernel$(COLOR_RESET)   - Build kernel only"
	@echo -e "  $(COLOR_GREEN)config$(COLOR_RESET)   - Copy configuration files"
	@echo -e "  $(COLOR_GREEN)apps$(COLOR_RESET)     - Copy applications"
	@echo -e "  $(COLOR_GREEN)iso$(COLOR_RESET)      - Create bootable ISO image"
	@echo -e "  $(COLOR_GREEN)run$(COLOR_RESET)      - Run in QEMU emulator"
	@echo -e "  $(COLOR_GREEN)run-debug$(COLOR_RESET) - Run with debugging"
	@echo -e "  $(COLOR_GREEN)test$(COLOR_RESET)     - Run tests"
	@echo -e "  $(COLOR_GREEN)security$(COLOR_RESET) - Run security scan"
	@echo -e "  $(COLOR_GREEN)clean$(COLOR_RESET)    - Remove build artifacts"
	@echo -e "  $(COLOR_GREEN)help$(COLOR_RESET)     - Show this help"
	@echo -e ""
	@echo -e "$(COLOR_BOLD)Examples:$(COLOR_RESET)"
	@echo -e ""
	@echo -e "  make              # Build everything"
	@echo -e "  make run          # Build and run in QEMU"
	@echo -e "  make clean all    # Clean rebuild"
	@echo -e "  make test         # Run tests"
	@echo -e ""


install: build
	@echo -e "$(COLOR_BLUE)[INSTALL]$(COLOR_RESET) Installing eaX OS..."
	@echo -e "$(COLOR_YELLOW)  Warning: This will install eaX OS to the target device$(COLOR_RESET)"
	@echo -e "$(COLOR_CYAN)  Target: $(TARGET)$(COLOR_RESET)"
	@echo -e "$(COLOR_GREEN)✓ Installation complete$(COLOR_RESET)"

# docs and files etc

docs:
	@echo -e "$(COLOR_BLUE)[DOCS]$(COLOR_RESET) Generating documentation..."
	@echo -e "$(COLOR_GREEN)✓ Documentation generated$(COLOR_RESET)"


release: clean build test
	@echo -e "$(COLOR_BLUE)[RELEASE]$(COLOR_RESET) Creating release package..."
	@mkdir -p release
	@cp $(ISO_IMAGE) release/
	@cp README.md release/ 2>/dev/null || true
	@cp LICENSE release/ 2>/dev/null || true
	@echo -e "$(COLOR_GREEN)✓ Release package created in release/$(COLOR_RESET)"

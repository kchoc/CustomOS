# Toolchain
CC = gcc
LD = ld
AS = nasm
CFLAGS = -m32 -Iinclude
LDFLAGS = -m elf_i386
ASMFLAGS = -f elf32

# Directories
SRC_DIR = src
INCLUDE_DIR = includ
BUILD_DIR = build

# Files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
LINKER_SCRIPT = linker.ld

# Source files
C_SOURCES = $(wildcard kernel.c $(SRC_DIR)/*.c)
ASM_SOURCES = $(wildcard $(SRC_DIR)/*.asm)

# Object files
OBJS = $(patsubst kernel.c, $(BUILD_DIR)/kernel.o, $(notdir $(C_SOURCES:.c=.o))) \
       $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) \
       $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))

# Rules
.PHONY: all clean

all: $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_ELF)
	i686-elf-objcopy -O binary $< $@

$(KERNEL_ELF): $(OBJS) $(LINKER_SCRIPT)
	$(LD) -T $(LINKER_SCRIPT) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: kernel.c
	$(CC) $(CFLAGS) -I $(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I $(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	$(AS) $(ASMFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*.o $(KERNEL_ELF) $(KERNEL_BIN)


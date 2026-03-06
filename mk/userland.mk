# ======================================
# Userland (CMake-equivalent + install)
# ======================================

USER_DIR        := user
USER_INC        := $(USER_DIR)/include
USER_LIB_DIR    := $(USER_DIR)/lib
USER_PROG_DIR   := $(USER_DIR)/programs

USER_BUILD_DIR  := build/user
USER_LIB_BUILD  := $(USER_BUILD_DIR)/lib

USER_LOAD_ADDR  := 0x00400000

# IMAGE must be defined in main Makefile
# (used for FAT copying)

# --------------------------------------
# Source discovery
# --------------------------------------

USER_ASM_SRCS := $(wildcard $(USER_PROG_DIR)/*.asm)
USER_C_SRCS   := $(wildcard $(USER_PROG_DIR)/*.c)
USER_LIB_SRCS := $(wildcard $(USER_LIB_DIR)/*.c)

# --------------------------------------
# Derived outputs
# --------------------------------------

USER_ASM_OBJS := $(patsubst $(USER_PROG_DIR)/%.asm,$(USER_BUILD_DIR)/%.o,$(USER_ASM_SRCS))
USER_ASM_ELFS := $(USER_ASM_OBJS:.o=.elf)

USER_LIB_OBJS := $(patsubst $(USER_LIB_DIR)/%.c,$(USER_LIB_BUILD)/%.o,$(USER_LIB_SRCS))

USER_C_ELFS   := $(patsubst $(USER_PROG_DIR)/%.c,$(USER_BUILD_DIR)/%.elf,$(USER_C_SRCS))

USERLAND_ELFS := $(USER_ASM_ELFS) $(USER_C_ELFS)

# Public variable for external use
USERLAND_BINS := $(USERLAND_ELFS)

# --------------------------------------
# Meta target
# --------------------------------------

userland: $(USERLAND_ELFS)

# --------------------------------------
# Create directories
# --------------------------------------

$(USER_BUILD_DIR):
	@mkdir -p $@

$(USER_LIB_BUILD):
	@mkdir -p $@

# --------------------------------------
# Build user library objects
# --------------------------------------

$(USER_LIB_BUILD)/%.o: $(USER_LIB_DIR)/%.c | $(USER_LIB_BUILD)
	$(CC) -m32 -ffreestanding -nostdlib \
	-I$(USER_INC) -c $< -o $@

# --------------------------------------
# Build ASM programs
# --------------------------------------

$(USER_BUILD_DIR)/%.o: $(USER_PROG_DIR)/%.asm | $(USER_BUILD_DIR)
	nasm -f elf32 $< -o $@

$(USER_BUILD_DIR)/%.elf: $(USER_BUILD_DIR)/%.o
	ld -m elf_i386 -Ttext $(USER_LOAD_ADDR) -e _start -o $@ $<

# --------------------------------------
# Build C programs
# --------------------------------------

$(USER_BUILD_DIR)/%.elf: $(USER_PROG_DIR)/%.c $(USER_LIB_OBJS) | $(USER_BUILD_DIR)
	$(CC) -m32 -ffreestanding -nostdlib -nostartfiles \
	-I$(USER_INC) \
	-Ttext=$(USER_LOAD_ADDR) \
	-o $@ $< $(USER_LIB_OBJS)

# --------------------------------------
# Install into FAT image
# --------------------------------------

install-userland: userland
	@echo "[IMG] Copying userland programs to FAT16"
	@for prog in $(USERLAND_ELFS); do \
		echo "  -> $$(basename $$prog)"; \
		mcopy -o -i "$(IMAGE)@@2048S" $$prog ::$$(basename $$prog); \
	done
	@echo "[IMG] Userland install complete."

# --------------------------------------
# Clean
# --------------------------------------

clean-userland:
	rm -rf $(USER_BUILD_DIR)
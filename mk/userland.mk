USER_DIR        := user
USER_INC        := $(USER_DIR)/include
USER_LIB_DIR    := $(USER_DIR)/lib
USER_PROG_DIR   := $(USER_DIR)/programs
USER_FILE_DIR		:= $(USER_DIR)/files

USER_BUILD_DIR  := build/user
USER_LIB_BUILD  := $(USER_BUILD_DIR)/lib

USER_LOAD_ADDR  := 0x00400000

USER_ASM_SRCS := $(wildcard $(USER_PROG_DIR)/*.asm)
USER_C_SRCS   := $(wildcard $(USER_PROG_DIR)/*.c)
USER_LIB_SRCS := $(wildcard $(USER_LIB_DIR)/*.c)
USER_FILES		:= $(wildcard $(USER_FILE_DIR)/*)

USER_ASM_OBJS := $(patsubst $(USER_PROG_DIR)/%.asm,$(USER_BUILD_DIR)/%.o,$(USER_ASM_SRCS))
USER_ASM_ELFS := $(USER_ASM_OBJS:.o=.elf)

USER_LIB_OBJS := $(patsubst $(USER_LIB_DIR)/%.c,$(USER_LIB_BUILD)/%.o,$(USER_LIB_SRCS))
USER_C_ELFS   := $(patsubst $(USER_PROG_DIR)/%.c,$(USER_BUILD_DIR)/%.elf,$(USER_C_SRCS))

USERLAND_ELFS := $(USER_ASM_ELFS) $(USER_C_ELFS)

userland: $(USERLAND_ELFS)

$(USER_LIB_BUILD)/%.o: $(USER_LIB_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -m32 -ffreestanding -nostdlib -I$(USER_INC) -c $< -o $@
 
$(USER_BUILD_DIR)/%.o: $(USER_PROG_DIR)/%.asm
	@mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@

$(USER_BUILD_DIR)/%.elf: $(USER_BUILD_DIR)/%.o
	ld -m elf_i386 -Ttext $(USER_LOAD_ADDR) -e _start -o $@ $<

$(USER_BUILD_DIR)/%.elf: $(USER_PROG_DIR)/%.c $(USER_LIB_OBJS)
	@mkdir -p $(@D)
	$(CC) -m32 -ffreestanding -nostdlib -nostartfiles \
	-I$(USER_INC) \
	-Ttext=$(USER_LOAD_ADDR) \
	-o $@ $< $(USER_LIB_OBJS)

system_init: userland $(DISK)
	@echo "Copying init.elf to disk image..."
	mcopy -o -i "$(DISK)@@2048S" $(USER_BUILD_DIR)/init.elf ::/sbin

install-userland: userland $(DISK)
	for prog in $(USERLAND_ELFS); do \
		mcopy -o -i "$(DISK)@@2048S" $$prog ::$$(basename $$prog); \
	done
	for file in $(USER_FILES); do \
		mcopy -o -i "$(DISK)@@2048S" $$file ::$$(basename $$file); \
	done

BOOT_BUILDDIR := $(BUILDDIR)/boot

BOOT0_SRC := $(BOOTDIR)/boot0.S
BOOT1_SRC := $(BOOTDIR)/boot1.S

BOOT0_OBJ := $(BOOT_BUILDDIR)/boot0.o
BOOT1_OBJ := $(BOOT_BUILDDIR)/boot1.o

BOOT0_BIN := $(BOOT_BUILDDIR)/boot0.bin
BOOT1_BIN := $(BOOT_BUILDDIR)/boot1.bin

KERNEL_ELF := $(BUILDDIR)/kernel.elf
KERNEL_BIN := $(BUILDDIR)/kernel.bin

IMAGE := $(BUILDDIR)/os.img

SECTOR_SIZE := 512
BOOT1_MAX_SECTORS := 1
KERNEL_SECTOR_START := 2049

KERNEL_SIZE = $(shell stat -c%s $(KERNEL_BIN) 2>/dev/null || echo 0)
KERNEL_SECTORS = $(shell echo $$(( ($(KERNEL_SIZE) + 511) / 512 )))

# --------------------------------------------------
# Stage 0 - boot0
# --------------------------------------------------

$(BOOT0_OBJ): $(BOOT0_SRC)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BOOT0_BIN): $(BOOT0_OBJ)
	ld -m elf_i386 -Ttext 0x7C00 --oformat binary $< -o $@
	# pad to 512 bytes exactly
	truncate -s $(SECTOR_SIZE) $@

# --------------------------------------------------
# Stage 1 - boot1
# --------------------------------------------------

$(BOOT1_OBJ): $(BOOT1_SRC) $(KERNEL_BIN)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -DKERNEL_SECTORS=$(KERNEL_SECTORS) -c $< -o $@

$(BOOT1_BIN): $(BOOT1_OBJ)
	ld -m elf_i386 -Ttext 0x7C00 --oformat binary $< -o $@
	# pad to 512 bytes exactly
	truncate -s $(SECTOR_SIZE) $@

# --------------------------------------------------
# Kernel binary
# --------------------------------------------------

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# --------------------------------------------------
# Sanity checks
# --------------------------------------------------

check-boot0:
	@size=$$(stat -c%s $(BOOT0_BIN)); \
	if [ $$size -ne $(SECTOR_SIZE) ]; then \
		echo "ERROR: boot0.bin size is $$size, expected $(SECTOR_SIZE) bytes"; \
		false; \
	fi

check-boot1:
	@size=$$(stat -c%s $(BOOT1_BIN)); \
	if [ $$size -gt $$(($(BOOT1_MAX_SECTORS)*$(SECTOR_SIZE))) ]; then \
		echo "ERROR: boot1.bin size is $$size, max allowed is $$(($(BOOT1_MAX_SECTORS)*$(SECTOR_SIZE))) bytes"; \
		false; \
	fi

# --------------------------------------------------
# Disk image
# --------------------------------------------------

$(IMAGE): $(BOOT0_BIN) $(BOOT1_BIN) $(KERNEL_BIN)
	rm -f $@

	# boot0 → LBA 0
	dd if=$(BOOT0_BIN) of=$@ bs=512 count=1 conv=notrunc

	# boot1 → LBA 2048
	dd if=$(BOOT1_BIN) of=$@ bs=512 seek=2048 conv=notrunc

	# kernel → LBA 2049
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=2049 conv=notrunc

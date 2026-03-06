BOOT_BUILDDIR := $(BUILDDIR)/boot

BOOT0_SRC := $(BOOTDIR)/boot0.S
BOOT1_SRC := $(BOOTDIR)/boot1.S
BOOT2_SRC := $(BOOTDIR)/boot2.S

BOOT0_OBJ := $(BOOT_BUILDDIR)/boot0.o
BOOT1_OBJ := $(BOOT_BUILDDIR)/boot1.o
BOOT2_OBJ := $(BOOT_BUILDDIR)/boot2.o

BOOT0_BIN := $(BOOT_BUILDDIR)/boot0.bin
BOOT1_BIN := $(BOOT_BUILDDIR)/boot1.bin
BOOT2_BIN := $(BOOT_BUILDDIR)/boot2.bin

KERNEL_ELF := $(BUILDDIR)/kernel.elf
KERNEL_BIN := $(BUILDDIR)/kernel.bin

IMAGE := $(BUILDDIR)/os.img

# After boot1.o is built, extract the address
LOAD_FILE_NM = $(shell nm --defined-only $(abspath $(BOOT1_OBJ)) | grep ' load_file')
LOAD_FILE_ADDR = $(shell echo $(LOAD_FILE_NM) | awk '{print "0x"$$1}')

SECTOR_SIZE := 512
IMAGE_SIZE_MB := 32

# --------------------------------------------------
# Stage 0 - boot0 (MBR)
# --------------------------------------------------

$(BOOT0_OBJ): $(BOOT0_SRC)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BOOT0_BIN): $(BOOT0_OBJ)
	ld -m elf_i386 -Ttext 0x7C00 --oformat binary $< -o $@

# --------------------------------------------------
# Stage 1 - boot1 (FAT16 VBR)
# --------------------------------------------------

$(BOOT1_OBJ): $(BOOT1_SRC)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BOOT1_BIN): $(BOOT1_OBJ)
	ld -m elf_i386 -Ttext 0x7C00 --oformat binary $< -o $@

# --------------------------------------------------
# Stage 2 - boot2 (kernel loader and environment setup)
# --------------------------------------------------
$(BOOT2_OBJ): $(BOOT2_SRC) $(BOOT1_OBJ)
	mkdir -p $(dir $@)
	@echo "Extracting load_file address from boot1.o: $(LOAD_FILE_ADDR)"
	$(CC) $(ASFLAGS) -DLOAD_FILE=$(LOAD_FILE_ADDR) -c $< -o $@

$(BOOT2_BIN): $(BOOT2_OBJ)
	ld -m elf_i386 -Ttext 0x7E00 --oformat binary $< -o $@

# --------------------------------------------------
# Check size constraints
# --------------------------------------------------

check_boot0_size:
	@BOOT0_SIZE=$$(stat -c%s $(BOOT0_BIN)); \
	if [ $$BOOT0_SIZE -gt $(SECTOR_SIZE) ]; then \
		echo "Error: boot0.bin is too large ($$BOOT0_SIZE bytes)"; \
		exit 1; \
	fi

check_boot1_size:
	@BOOT1_SIZE=$$(stat -c%s $(BOOT1_BIN)); \
	if [ $$BOOT1_SIZE -gt $(SECTOR_SIZE) ]; then \
		echo "Error: boot1.bin is too large ($$BOOT1_SIZE bytes)"; \
		exit 1; \
	fi

# --------------------------------------------------
# Kernel binary
# --------------------------------------------------

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# --------------------------------------------------
# Disk Image (Option B: MBR + FAT16 at LBA 1)
# --------------------------------------------------

$(IMAGE): $(BOOT0_BIN) $(BOOT1_BIN) $(BOOT2_BIN) $(KERNEL_BIN) check_boot0_size check_boot1_size
	@echo "[IMG] Creating raw FAT16 disk (no mkfs)"
	@{ \
		set -e; \
		\
		rm -f $(IMAGE); \
		echo "[IMG] Create blank image"; \
		truncate -s $(IMAGE_SIZE_MB)M $(IMAGE); \
		\
		echo "[IMG] Write boot0 @ LBA0"; \
		dd if=$(BOOT0_BIN) of=$(IMAGE) bs=512 count=1 conv=notrunc; \
		\
		echo "[IMG] mformat FAT16 at LBA2048"; \
		mformat -i "$(IMAGE)@@2048S" -H 2048 ::; \
		\
		echo "[IMG] Adding files to FAT16"; \
		mcopy -i "$(IMAGE)@@2048S" $(BOOT2_BIN) ::; \
		mcopy -i "$(IMAGE)@@2048S" $(KERNEL_BIN) ::; \
		mcopy -i "$(IMAGE)@@2048S" test.txt ::; \
# 		$(MAKE) install-userland IMAGE=$(IMAGE); \
		\
		echo "[IMG] Write boot1 (BPB) @ LBA2048"; \
		dd if=$(BOOT1_BIN) of=$(IMAGE) bs=512 seek=2048 conv=notrunc; \
		\
		echo "[IMG] Done."; \
	}
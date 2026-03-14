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

IMAGE := $(BUILDDIR)/os.img
DISK  := $(BUILDDIR)/disk.img

SECTOR_SIZE := 512
IMAGE_SIZE_MB := 32

LOAD_FILE_NM = $(shell nm --defined-only $(abspath $(BOOT1_OBJ)) | grep ' load_file')
LOAD_FILE_ADDR = $(shell echo $(LOAD_FILE_NM) | awk '{print "0x"$$1}')

$(BOOT0_OBJ): $(BOOT0_SRC)
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BOOT1_OBJ): $(BOOT1_SRC)
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BOOT2_OBJ): $(BOOT2_SRC) $(BOOT1_OBJ)
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -DLOAD_FILE=$(LOAD_FILE_ADDR) -c $< -o $@

$(BOOT0_BIN): $(BOOT0_OBJ)
	ld -m elf_i386 -Ttext 0x7C00 --oformat binary $< -o $@

$(BOOT1_BIN): $(BOOT1_OBJ)
	ld -m elf_i386 -Ttext 0x7C00 --oformat binary $< -o $@

$(BOOT2_BIN): $(BOOT2_OBJ)
	ld -m elf_i386 -Ttext 0x7E00 --oformat binary $< -o $@

$(DISK):
	truncate -s $(IMAGE_SIZE_MB)M $@
	mformat -i "$@@@2048S" -H 2048 ::

copy-files: $(BOOT2_BIN) $(KERNEL_BIN) $(DISK)
	mcopy -o -i "$(DISK)@@2048S" $(BOOT2_BIN) ::boot2.bin
	mcopy -o -i "$(DISK)@@2048S" $(KERNEL_BIN) ::kernel.bin

$(IMAGE): $(DISK) copy-files $(BOOT0_BIN) $(BOOT1_BIN) system_init
	cp $(DISK) $@
	dd if=$(BOOT0_BIN) of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BOOT1_BIN) of=$@ bs=512 seek=2048 conv=notrunc

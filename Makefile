ARCH ?= i386
BUILDDIR := build

include mk/toolchain.mk
include mk/arch-$(ARCH).mk
include mk/flags.mk
include mk/headers.mk
include mk/kernel.mk
include mk/boot.mk
include mk/rules.mk
include mk/userland.mk

RUN:= ${QEMU} -cpu $(QEMU_CPU) -m 512M -serial file:serial.log -D qemu.log -no-reboot
GDB ?= gdb
GDB_PORT ?= 1234

all: $(IMAGE)

run: $(IMAGE)
	$(RUN) -hda $< -d cpu_reset,guest_errors

debug: $(IMAGE)
	$(RUN) -hda $< -d cpu_reset,in_asm,int

gdb: $(IMAGE)
	$(RUN) -hda $< -d cpu_reset,in_asm,int -S -s

clean:
	rm -rf $(BUILDDIR)

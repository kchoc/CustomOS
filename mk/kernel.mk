#----------------------------------------
# kernel.mk
#----------------------------------------

ARCH     := i386
BUILDDIR := build

#----------------------------------------
# Source directories
#----------------------------------------
KERNEL_MI_DIRS := \
	sys/kern \
	sys/dev \
	sys/sys \
	sys/fs \
	sys/disk \
	sys/libkern \
	sys/vm \
	sys/wm

# ARCH_SRCDIRS is expected to be set before
# this file is included, e.g.:
#   ARCH_SRCDIRS := sys/i386 sys/i386/i386
KERNEL_MD_DIRS := $(ARCH_SRCDIRS)

#----------------------------------------
# Recursive wildcard helper
#----------------------------------------
rwildcard = \
	$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)) \
	$(filter $(subst *,%,$2),$(wildcard $1$2))

#----------------------------------------
# genassym — must come BEFORE kernel objects
#
# Dependency chain:
#   headers (symlinks) --> genassym_prog --> genassym.h --> kernel objects
#
# genassym_prog depends only on the specific
# machine/ symlinks it actually #includes,
# NOT on the whole 'headers' phony target.
# This is what breaks the circular dependency.
#----------------------------------------
GENASSYM_SRC  := sys/$(ARCH)/include/genassym.c
GENASSYM_PROG := $(BUILDDIR)/genassym_prog
GENASSYM_H    := $(MACHINE_DIR)/genassym.h

# List every machine/ header that genassym.c
# transitively includes. Add more here if the
# compiler complains about a missing header.
GENASSYM_HDR_DEPS := \
	$(MACHINE_DIR)/segment.h

# Build genassym_prog only after its required
# symlinks exist. Does NOT depend on 'headers'.
$(GENASSYM_PROG): $(GENASSYM_SRC) $(GENASSYM_HDR_DEPS) | $(MACHINE_DIR)
	$(CC) -m32 -o $@ $< $(INCLUDES) -D__GENASSYM__

# Run the program to emit the header
$(GENASSYM_H): $(GENASSYM_PROG) | $(MACHINE_DIR)
	$< > $@

#----------------------------------------
# Kernel sources
#----------------------------------------
KERNEL_C_SRCS := \
	$(foreach d,$(KERNEL_MI_DIRS),$(call rwildcard,$(d)/,*.c)) \
	$(foreach d,$(KERNEL_MD_DIRS),$(call rwildcard,$(d)/,*.c))

KERNEL_S_SRCS := \
	$(foreach d,$(KERNEL_MI_DIRS),$(call rwildcard,$(d)/,*.S)) \
	$(foreach d,$(KERNEL_MD_DIRS),$(call rwildcard,$(d)/,*.S))

KERNEL_SRCS := $(KERNEL_C_SRCS) $(KERNEL_S_SRCS)

#----------------------------------------
# Kernel objects
#----------------------------------------
KERNEL_OBJS := $(addprefix $(BUILDDIR)/,$(KERNEL_SRCS:.c=.o))
KERNEL_OBJS := $(KERNEL_OBJS:.S=.o)

# Early objects must be linked first (boot / low-level asm)
EARLY_KERNEL_SRCS := \
	sys/$(ARCH)/$(ARCH)/locore.S \
	sys/$(ARCH)/$(ARCH)/support.S

EARLY_OBJS := $(addprefix $(BUILDDIR)/,$(EARLY_KERNEL_SRCS:.S=.o))
LATE_OBJS  := $(filter-out $(EARLY_OBJS),$(KERNEL_OBJS))

#----------------------------------------
# Compilation rules
#----------------------------------------

# All kernel objects depend on generated header
# and the full set of machine/ symlinks.
$(KERNEL_OBJS): $(GENASSYM_H) $(MACHINE_LINKS)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -D__ASSEMBLER__ -c $< -o $@

#----------------------------------------
# ELF / Binary targets
#----------------------------------------
KERNEL_ELF := $(BUILDDIR)/kernel.elf
KERNEL_BIN := $(BUILDDIR)/kernel.bin

# Link: early objs first, then the rest
$(KERNEL_ELF): $(EARLY_OBJS) $(LATE_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(EARLY_OBJS) $(LATE_OBJS) $(LATE_ARCH_LDFLAGS)

# Strip to raw binary for bootloader
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

#----------------------------------------
# Convenience targets
#----------------------------------------
.PHONY: kernel
kernel: $(KERNEL_BIN)


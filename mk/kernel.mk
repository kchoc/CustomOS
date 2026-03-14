#----------------------------------------
# Directories
#----------------------------------------
ARCH := i386
BUILDDIR := build

KERNEL_MI_DIRS := \
	sys/kern \
	sys/dev \
	sys/sys \
	sys/fs \
	sys/disk \
	sys/libkern \
	sys/vm \
	sys/wm

KERNEL_MD_DIRS := $(ARCH_SRCDIRS)  # set elsewhere, e.g., sys/i386

#----------------------------------------
# genassym
#----------------------------------------
GENASSYM_SRC := sys/$(ARCH)/include/genassym.c
GENASSYM_H   := $(BUILDDIR)/include/machine/genassym.h
GENASSYM_PROG := $(BUILDDIR)/genassym_prog

# Compile and run genassym.c to generate genassym.h
$(GENASSYM_H): $(GENASSYM_SRC)
	@mkdir -p $(dir $@)
	$(CC) -m32 -o $(GENASSYM_PROG) $(GENASSYM_SRC) $(INCLUDES) -D__GENASSYM__
	$(GENASSYM_PROG) > $@

#----------------------------------------
# Recursive wildcard helper
#----------------------------------------
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)) \
            $(filter $(subst *,%,$2),$(wildcard $1$2))

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

EARLY_KERNEL_OBJS := \
	sys/$(ARCH)/$(ARCH)/locore.S \
	sys/$(ARCH)/$(ARCH)/support.S

EARLY_OBJS := $(addprefix $(BUILDDIR)/,$(EARLY_KERNEL_OBJS:.S=.o))
LATE_OBJS  := $(filter-out $(EARLY_OBJS),$(KERNEL_OBJS))

#----------------------------------------
# ELF / Binary
#----------------------------------------
KERNEL_ELF := $(BUILDDIR)/kernel.elf
KERNEL_BIN := $(BUILDDIR)/kernel.bin

# Ensure all kernel objects depend on genassym.h
$(KERNEL_OBJS): $(GENASSYM_H)

# Link kernel ELF
$(KERNEL_ELF): headers $(EARLY_OBJS) $(LATE_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(EARLY_OBJS) $(LATE_OBJS) $(LATE_ARCH_LDFLAGS)

# Create binary
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@


KERNEL_MI_DIRS := \
	sys/kern \
	sys/dev \
	sys/fs \
	sys/libkern \
	sys/vm \
	sys/wm

KERNEL_MD_DIRS := $(ARCH_SRCDIRS)

KERNEL_C_SRCS := \
	$(foreach d,$(KERNEL_MI_DIRS),$(shell find $(d) -name '*.c')) \
	$(foreach d,$(KERNEL_MD_DIRS),$(shell find $(d) -name '*.c'))

KERNEL_S_SRCS := \
	$(foreach d,$(KERNEL_MI_DIRS),$(shell find $(d) -name '*.S')) \
	$(foreach d,$(KERNEL_MD_DIRS),$(shell find $(d) -name '*.S'))

KERNEL_SRCS := $(KERNEL_C_SRCS) $(KERNEL_S_SRCS)

KERNEL_OBJS := $(addprefix $(BUILDDIR)/,$(KERNEL_SRCS:.c=.o))
KERNEL_OBJS := $(KERNEL_OBJS:.S=.o)

EARLY_KERNEL_OBJS := \
	sys/$(ARCH)/$(ARCH)/locore.S \
	sys/$(ARCH)/$(ARCH)/support.S

EARLY_OBJS := $(addprefix $(BUILDDIR)/,$(EARLY_KERNEL_OBJS:.S=.o))
LATE_OBJS  := $(filter-out $(EARLY_OBJS),$(KERNEL_OBJS))

KERNEL_ELF := $(BUILDDIR)/kernel.elf

$(KERNEL_ELF): $(HDR_STAMP) $(EARLY_OBJS) $(LATE_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(EARLY_OBJS) $(LATE_OBJS)

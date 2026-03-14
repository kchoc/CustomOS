INCDIR := $(BUILDDIR)/include
MACHINE_DIR := $(INCDIR)/machine

ARCH_MACHINE_HEADERS := $(wildcard sys/$(ARCH)/include/*.h)
X86_MACHINE_HEADERS  := $(wildcard sys/x86/include/*.h)

MACHINE_HEADERS := $(notdir $(ARCH_MACHINE_HEADERS) $(X86_MACHINE_HEADERS))

INCLUDES := \
	-Isys \
	-Iinclude \
	-I$(INCDIR) \
	$(foreach d,$(ARCH_SRCDIRS),-I$(d))

$(MACHINE_DIR):
	@mkdir -p $@

define MACHINE_HEADER_RULE
$(MACHINE_DIR)/$(1): | $(MACHINE_DIR)
	@if [ -f sys/$(ARCH)/include/$(1) ]; then \
		ln -sf $(abspath sys/$(ARCH)/include/$(1)) $$@; \
	else \
		ln -sf $(abspath sys/x86/include/$(1)) $$@; \
	fi
endef

$(foreach h,$(MACHINE_HEADERS),$(eval $(call MACHINE_HEADER_RULE,$(h))))

MACHINE_LINKS := $(addprefix $(MACHINE_DIR)/,$(MACHINE_HEADERS))

headers: $(MACHINE_LINKS)
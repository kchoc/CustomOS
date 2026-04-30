#----------------------------------------
# headers.mk
# Sets up build/include/machine/ symlinks
# pointing to arch-specific headers.
#
# Priority: ARCH_INC (e.g. sys/i386/include)
#           overrides X86_INC (sys/x86/include)
#           when both provide the same header.
#
# NOTE: genassym.h is intentionally excluded
#       from the headers target. It is generated
#       by kernel.mk after genassym_prog runs.
#----------------------------------------

INCDIR      := $(BUILDDIR)/include
MACHINE_DIR := $(INCDIR)/machine

ARCH_INC := sys/$(ARCH)/include
X86_INC  := sys/x86/include

ARCH_MACHINE_HEADERS := $(wildcard $(ARCH_INC)/*.h)
X86_MACHINE_HEADERS  := $(wildcard $(X86_INC)/*.h)

# Merge header lists; ARCH takes priority via the rule logic below.
# Sort deduplicates names that appear in both directories.
MACHINE_HEADERS := $(sort \
	$(notdir $(X86_MACHINE_HEADERS)) \
	$(notdir $(ARCH_MACHINE_HEADERS)))

# Exclude genassym.h — it is generated, not symlinked
MACHINE_HEADERS := $(filter-out genassym.h,$(MACHINE_HEADERS))

INCLUDES := \
	-Iinclude \
	-Isys \
	-I$(INCDIR) \
	-I$(ARCH_INC) \
	-I$(X86_INC)

#----------------------------------------
# Create the machine/ directory
#----------------------------------------
$(MACHINE_DIR):
	@mkdir -p $@

#----------------------------------------
# Per-header symlink rules
# ARCH_INC wins over X86_INC when both exist.
#----------------------------------------
define MACHINE_HEADER_RULE
$(MACHINE_DIR)/$(1): | $(MACHINE_DIR)
	@if [ -f $(ARCH_INC)/$(1) ]; then \
		ln -sf $(abspath $(ARCH_INC)/$(1)) $$@; \
	else \
		ln -sf $(abspath $(X86_INC)/$(1)) $$@; \
	fi
endef

$(foreach h,$(MACHINE_HEADERS),$(eval $(call MACHINE_HEADER_RULE,$(h))))

MACHINE_LINKS := $(addprefix $(MACHINE_DIR)/,$(MACHINE_HEADERS))

#----------------------------------------
# headers target
# Only symlinks — genassym.h is NOT listed
# here to avoid a circular dependency with
# the genassym_prog build in kernel.mk.
#----------------------------------------
.PHONY: headers
headers: $(MACHINE_LINKS)

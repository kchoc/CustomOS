INCDIR := $(BUILDDIR)/include
HDR_STAMP := $(INCDIR)/.installed

HDR_DIRS := \
	sys/kern \
	sys/dev \
	sys/fs \
	sys/libkern \
	sys/vm \
	sys/wm \
	sys/sys \
	sys/x86 \
	include \
	sys/$(ARCH)

INCLUDES := -I$(INCDIR) -I$(INCDIR)/sys -I$(INCDIR)/include

$(HDR_STAMP):
	@echo "Staging headers"
	rm -rf $(INCDIR)
	mkdir -p $(INCDIR)/machine
	for d in $(HDR_DIRS); do \
		find $$d -name '*.h' -exec cp --parents {} $(INCDIR) \; ; \
	done
	for d in $(ARCH_HDRDIRS); do \
		cp $$d/*.h $(INCDIR)/machine; \
	done
	touch $@

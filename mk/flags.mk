COMMON_CFLAGS := \
	-ffreestanding \
	-fno-builtin \
	-fno-stack-protector \
	-fno-pie \
	-no-pie \
	-nostdlib \
	-O2 \
	-Wall \
	-Wextra \
	-fno-asynchronous-unwind-tables\
	-fno-unwind-tables \
	-fno-exceptions


CFLAGS  := $(ARCH_CFLAGS) $(COMMON_CFLAGS) -MMD -MP
ASFLAGS := $(ARCH_CFLAGS) -x assembler-with-cpp -MMD -MP
LDFLAGS := $(ARCH_LDFLAGS) -nostdlib -no-pie -Wl,-T,linker.ld

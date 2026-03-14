COMMON_CFLAGS := \
	-ffreestanding \
	-fno-stack-protector \
	-fno-pie \
	-nostdlib \
	-O2 \
	-Wall \
	-Wextra

CFLAGS  := $(ARCH_CFLAGS) $(COMMON_CFLAGS) -MMD -MP
ASFLAGS := $(ARCH_CFLAGS) -x assembler-with-cpp -MMD -MP
LDFLAGS := $(ARCH_LDFLAGS) -nostdlib -no-pie -Wl,-T,linker.ld

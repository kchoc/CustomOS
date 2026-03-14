ARCH_CFLAGS  := -m32
ARCH_LDFLAGS := -m32
LATE_ARCH_LDFLAGS := -lgcc

QEMU     := qemu-system-i386
QEMU_CPU := pentium3

BOOTDIR := boot/i386

ARCH_SRCDIRS := \
	sys/i386 \
	sys/x86
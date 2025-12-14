# CustomOSC - Custom Operating System Kernel

⚠️ **EXPERIMENTAL PROJECT - FOR EDUCATIONAL PURPOSES ONLY** ⚠️

CustomOSC is a 32-bit IA-32 OS kernel written in C and assembly, demonstrating concepts like boot processes, memory/process management, SMP, device drivers, filesystems, interrupts, and system calls. **Highly unstable, not for production use.**

## Prerequisites

- **GCC i686-elf**, **NASM**, **GRUB utilities**, **QEMU**, **Linux/WSL**
- Install on Ubuntu/Debian:
   ```bash
   sudo apt update && sudo apt install build-essential cmake nasm grub-pc-bin xorriso qemu-system-x86
   ```

## Build & Run

1. Clone the repo and enter the project directory.
2. Create a small filesystem image (if needed):
   ```bash
   dd if=/dev/zero of=fs.img bs=1M count=16
   ```
3. Configure and build using CMake. From the project root:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make
   ```
4. Run the built ISO using the included Makefile target:
   ```bash
   make run
   ```

The `make run` target runs QEMU with the built ISO and the `fs.img` disk image. If you prefer to run QEMU manually, use the previous QEMU invocation with `-cdrom my-kernel.iso` and `-drive file=fs.img,format=raw`.

Additional CMake/Make targets available in the build directory:

- `make cleardrive` — clear or recreate the `fs.img` image used as the guest drive (if supported by the CMake scripts).
- `make userland` — build userland programs and utilities (rebuilds the `user/` programs).
- `make install_programs` — install/copy built user programs into the ISO filesystem image so they're available to the kernel at runtime.

Use these from the project root with the top-level wrapper Makefile or by running them inside `build/`, e.g.:
```bash
make cleardrive     # clears/recreates fs.img
make userland       # builds user programs
make install_programs
make run
```

## Features (Experimental)

- GRUB bootloader, terminal, memory/process management, interrupts, drivers, system calls, virtual filesystem.

## Known Issues

- Crashes, memory leaks, limited hardware support, race conditions.

## Learning Objectives

Understand OS architecture, low-level programming, memory/process management, and hardware abstraction.

## Disclaimer

**USE AT YOUR OWN RISK.** This project is for educational purposes only. Not suitable for real-world use.

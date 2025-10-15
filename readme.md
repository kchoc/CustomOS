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

1. Clone the repo, navigate to the directory.
2. Construct filesystem image:
   ```bash
   dd if=/dev/zero of=fs.img bs=1M count=16
   ```
2. Build and run with QEMU:
    ```bash
   ./run.bash
    ```

## Features (Experimental)

- GRUB bootloader, terminal, memory/process management, interrupts, drivers, system calls, virtual filesystem.

## Known Issues

- Crashes, memory leaks, limited hardware support, race conditions.

## Learning Objectives

Understand OS architecture, low-level programming, memory/process management, and hardware abstraction.

## Disclaimer

**USE AT YOUR OWN RISK.** This project is for educational purposes only. Not suitable for real-world use.

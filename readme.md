# CustomOSC - Custom Operating System Kernel + Shell

A custom operating system kernel built from scratch, designed to be lightweight and simple. The kernel is written in C and Assembly, and it provides basic functionality for process management, memory management, and file system support. The shell is a simple command-line interface that allows users to interact with the operating system.

The project architecture is highly inspired by FreeBSD but is designed to be much simpler. This often results in simplicity being prioritised over performance.
## Features
* Virtual File System supporting Devices, Fat16, MBR, and Sockets (soon)
* Multitasking and task scheduling
* Forking and Exec
* Support of i386 with machine independant code seperated for future multi-arch support
* Teletype Terminal output
* Sophisticated Virtual Memory system architecture
## Build & Run

Run the built ISO using the included Makefile target:
   ```bash
   make headers
   make install-userland
   make run
   ```

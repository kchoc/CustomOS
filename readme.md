# CustomOSC - Custom Operating System Kernel

**EXPERIMENTAL BRANCH**

This branch is an effort to reconstruct the original kernel project found on the stable branch to support multiple architectures. This branch is also designed to move away from the GRUB bootloader, convert all assembly to GAS and move from cmake to make
 
## Build & Run

Run the built ISO using the included Makefile target:
   ```bash
   make run
   ```

## Stable Single-Arch

The original i386 kernel can be found on the stable branch.
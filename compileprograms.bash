umount ./fat16
mount -t vfat fs.img ./fat16 -o shortname=win95
nasm -f elf32 -o hello.o ./user/programs/hello.asm
ld -m elf_i386 -Ttext 0x00001000 -e _start -o ./fat16/hello.elf hello.o
mkdir build

nasm -f elf32 src/boot.asm -o build/boot.o

for file in $(find src -name '*.c'); do
    gcc -m32 -Iinclude -c $file -o build/$(basename $file .c).o
done

OBJECTS="build/boot.o build/kernel.o $(ls build/*.o | grep -v boot.o | grep -v kernel.o)"
ld -m elf_i386 -T linker.ld -o kernel $OBJECTS

cp kernel iso/boot/
cp grub.cfg iso/boot/grub/
grub-mkrescue -o my-kernel.iso iso/

rm $OBJECTS kernel

qemu-system-i386 -cdrom my-kernel.iso

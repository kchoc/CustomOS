mkdir build

for file in $(find src -name '*.c'); do
    gcc -m32 -ffreestanding -fno-stack-protector -Iinclude -c $file -o build/$(basename $file .c).o
done

for file in $(find src -name '*.asm'); do
    nasm -f elf32 $file -o build/$(basename $file .asm).o
done

OBJECTS=$(find build -name '*.o')
ld -m elf_i386 -T linker.ld -o kernel $OBJECTS

cp kernel iso/boot/
cp grub.cfg iso/boot/grub/
grub-mkrescue -o my-kernel.iso iso/

rm $OBJECTS kernel

qemu-system-i386 -cdrom my-kernel.iso

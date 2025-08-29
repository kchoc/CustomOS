for file in $(find src -name '*.c'); do
    gcc -m32 -march=i686 -ffreestanding -fno-stack-protector -Iinclude -c $file -o build/$(basename $file .c).o
done

for file in $(find src -name '*.asm'); do
    nasm -f elf32 $file -o build/$(basename $file .asm).o
done

for file in $(find src -name '*.s'); do
    gcc -m32 -march=i686 -ffreestanding -fno-stack-protector -Iinclude -c $file -o build/$(basename $file .s).o
done

OBJECTS=$(find build -name '*.o')
ld -m elf_i386 -T linker.ld -o kernel $OBJECTS

cp kernel iso/boot/
cp grub.cfg iso/boot/grub/
grub-mkrescue -o my-kernel.iso iso/

rm $OBJECTS kernel

qemu-system-i386 \
  -cpu pentium3 \
  -boot d \
  -cdrom my-kernel.iso \
  -drive file=fs.img,format=raw,index=0,media=disk \
  -m 4G

umount ./fat16
dd if=/dev/zero of=fs.img bs=1M count=64
mkdir -p ./fat16

# Mount needs to be done manually later as the kernel formats the disk
# sudo mount -t vfat fs.img ./fat16 -o shortname=win95

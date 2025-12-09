#!/bin/bash

# This script builds userland programs and copies them to the FAT16 filesystem
# Now integrated with CMake - this is just a convenience wrapper

cd build

echo "Building userland programs with CMake..."
cmake --build . --target userland

echo ""
echo "Installing programs to FAT16 filesystem..."
sudo cmake --build . --target install_programs

echo ""
echo "Available programs:"
ls -lh user/*.elf 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
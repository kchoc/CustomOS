cmake_minimum_required(VERSION 3.20)
project(CustomOSKernel C ASM)

# Enable ASM languages
enable_language(ASM_NASM)
enable_language(ASM)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --------------------------
# Source files
# --------------------------
file(GLOB_RECURSE KERNEL_C_SRC ${CMAKE_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE NASM_SRC ${CMAKE_SOURCE_DIR}/src/*.asm)
file(GLOB_RECURSE GAS_SRC  ${CMAKE_SOURCE_DIR}/src/*.s)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --------------------------
# Build trampoline.bin -> trampoline.o
# --------------------------
set(TRAMPOLINE_SRC ${CMAKE_SOURCE_DIR}/src/kernel/process/trampoline.asm)
set(TRAMPOLINE_BIN ${CMAKE_BINARY_DIR}/trampoline.bin)
set(TRAMPOLINE_O   ${CMAKE_BINARY_DIR}/trampoline.o)

add_custom_command(
    OUTPUT ${TRAMPOLINE_O}
    COMMAND nasm -f bin ${TRAMPOLINE_SRC} -o ${TRAMPOLINE_BIN}
    COMMAND ld -r -b binary ${TRAMPOLINE_BIN} -o ${TRAMPOLINE_O}
    DEPENDS ${TRAMPOLINE_SRC}
    COMMENT "Building trampoline binary"
)

# --------------------------
# NASM compilation
# --------------------------
# Exclude trampoline.asm
file(GLOB_RECURSE NASM_SRC_TEMP ${CMAKE_SOURCE_DIR}/src/*.asm)
set(NASM_SRC "")
foreach(nasm_file IN LISTS NASM_SRC_TEMP)
    if(NOT nasm_file STREQUAL TRAMPOLINE_SRC)
        list(APPEND NASM_SRC ${nasm_file})
    endif()
endforeach()

# Create object files for NASM sources manually
set(NASM_OBJ "")
foreach(nasm_file IN LISTS NASM_SRC)
    get_filename_component(nasm_name ${nasm_file} NAME_WE)
    set(nasm_o ${CMAKE_BINARY_DIR}/${nasm_name}.o)
    add_custom_command(
        OUTPUT ${nasm_o}
        COMMAND nasm -f elf32 ${nasm_file} -o ${nasm_o}
        DEPENDS ${nasm_file}
        COMMENT "Assembling NASM file ${nasm_file}"
    )
    list(APPEND NASM_OBJ ${nasm_o})
endforeach()


# --------------------------
# Kernel ELF target
# --------------------------
add_executable(kernel.elf
    ${KERNEL_C_SRC}
    ${GAS_SRC}
    ${NASM_OBJ}
    ${TRAMPOLINE_O}
)

# --------------------------
# Compiler flags for C only
# --------------------------
# C flags
target_compile_options(kernel.elf PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-m32 -ffreestanding -O2 -Wall -Wextra>
)

# ASM flags (GAS)
set_source_files_properties(${KERNEL_ASM_SRC} PROPERTIES
    COMPILE_FLAGS "-m32"
)

# NASM files are handled in the custom commands already


# --------------------------
# Linker flags
# --------------------------
set_target_properties(kernel.elf PROPERTIES
    LINK_FLAGS "-m elf_i386 -T ${CMAKE_SOURCE_DIR}/linker.ld -nostdlib"
)

# --------------------------
# Bootable ISO
# --------------------------
add_custom_command(TARGET kernel.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/iso/boot/grub
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${CMAKE_BINARY_DIR}/iso/boot/kernel.elf
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/iso/grub.cfg ${CMAKE_BINARY_DIR}/iso/boot/grub/
    COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/my-kernel.iso ${CMAKE_BINARY_DIR}/iso
    COMMENT "Creating bootable ISO"
)

# --------------------------
# Run in QEMU
# --------------------------
add_custom_target(run
    COMMAND qemu-system-i386 -smp 4 -cpu pentium3 -boot d -cdrom ${CMAKE_BINARY_DIR}/my-kernel.iso -drive file=${CMAKE_SOURCE_DIR}/fs.img,format=raw,index=0,media=disk -m 4G
    DEPENDS kernel.elf
)

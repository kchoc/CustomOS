cmake_minimum_required(VERSION 3.20)
project(CustomOSKernel C ASM)

# --- Toolchain setup ---
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Use 32-bit cross GCC toolchain
set(CMAKE_C_COMPILER gcc)
set(CMAKE_ASM_COMPILER gcc)

# --- Global build flags ---
set(COMMON_FLAGS "-m32 -march=i686 -ffreestanding -fno-builtin -fno-stack-protector -I${CMAKE_SOURCE_DIR}/include")

set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${COMMON_FLAGS}")

# --- Gather all sources recursively ---
file(GLOB_RECURSE C_SOURCES CONFIGURE_DEPENDS src/*.c)
file(GLOB_RECURSE ASM_SOURCES CONFIGURE_DEPENDS src/*.s src/*.S)
file(GLOB_RECURSE NASM_SOURCES CONFIGURE_DEPENDS src/*.asm)

# --- NASM objects (need custom rule because CMake doesn't natively support NASM) ---
set(NASM_OBJECTS "")
foreach(nasm_file ${NASM_SOURCES})
    get_filename_component(nasm_filename ${nasm_file} NAME_WE)
    set(nasm_object "${CMAKE_BINARY_DIR}/${nasm_filename}.o")
    add_custom_command(
        OUTPUT ${nasm_object}
        COMMAND nasm -f elf32 -o ${nasm_object} ${nasm_file}
        DEPENDS ${nasm_file}
        COMMENT "Assembling ${nasm_file} to ${nasm_object}"
    )
    list(APPEND NASM_OBJECTS ${nasm_object})
endforeach()

# --- Create the kernel executable ---
add_executable(kernel ${C_SOURCES} ${ASM_SOURCES} ${NASM_OBJECTS})

set_target_properties(kernel PROPERTIES
    LINKER_LANGUAGE C
    LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/linker.ld -m32 -nostdlib -static -no-pie"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# --- Post-build: Create ISO image ---
add_custom_command(TARGET kernel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/iso/boot/grub
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/kernel ${CMAKE_BINARY_DIR}/iso/boot/kernel
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/grub.cfg ${CMAKE_BINARY_DIR}/iso/boot/grub/grub.cfg
    COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/my-kernel.iso ${CMAKE_BINARY_DIR}/iso
    COMMENT "Creating bootable ISO image at ${CMAKE_BINARY_DIR}/my-kernel.iso"
)

# --- Optional: Initiate QEMU for testing ---
add_custom_target(run
    COMMAND qemu-system-i386 -smp 4 -cpu pentium3 -boot d -cdrom my-kernel.iso -drive file=../fs.img,format=raw,index=0,media=disk -m 4G -d cpu_reset,int -D qemu.log
    DEPENDS kernel
    COMMENT "Starting QEMU with the built ISO"
)

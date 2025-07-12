cmake_minimum_required(VERSION 3.10)
project(MyKernel C ASM)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_ASM_COMPILER gcc)  # GAS for `.s` files
set(CMAKE_ASM_NASM_COMPILER nasm)  # NASM for `.asm` files

set(CMAKE_C_FLAGS "-m32 -ffreestanding -fno-stack-protector -Iinclude")
set(CMAKE_ASM_FLAGS "-m32 -ffreestanding -fno-stack-protector")  # GAS Flags
set(CMAKE_ASM_NASM_FLAGS "-f elf32")  # NASM Flags

include_directories(${CMAKE_SOURCE_DIR}/include)

# Get source files
file(GLOB_RECURSE C_SOURCES src/*.c)
file(GLOB_RECURSE GAS_SOURCES src/*.s)
file(GLOB_RECURSE NASM_SOURCES src/*.asm)

# Compile NASM `.asm` files separately
set(NASM_OBJECTS "")
foreach(ASM_FILE ${NASM_SOURCES})
    get_filename_component(FILENAME ${ASM_FILE} NAME_WE)
    set(OBJ_FILE ${CMAKE_BINARY_DIR}/${FILENAME}.o)
    add_custom_command(
        OUTPUT ${OBJ_FILE}
        COMMAND ${CMAKE_ASM_NASM_COMPILER} ${CMAKE_ASM_NASM_FLAGS} ${ASM_FILE} -o ${OBJ_FILE}
        DEPENDS ${ASM_FILE}
    )
    list(APPEND NASM_OBJECTS ${OBJ_FILE})
endforeach()

# Compile GAS `.s` files using gcc
add_executable(kernel ${C_SOURCES} ${GAS_SOURCES} ${NASM_OBJECTS})
set_target_properties(kernel PROPERTIES LINKER_LANGUAGE C)

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker.ld)
set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386 -T ${LINKER_SCRIPT}")

# Post-build actions
add_custom_command(TARGET kernel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> iso/boot/
    COMMAND ${CMAKE_COMMAND} -E copy grub.cfg iso/boot/grub/
    COMMAND grub-mkrescue -o my-kernel.iso iso/
    COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:kernel>
    COMMAND qemu-system-i386 -cdrom my-kernel.iso -m 4G
)

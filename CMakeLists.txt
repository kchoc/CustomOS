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

# --- Userland programs ---
file(GLOB USER_ASM_PROGRAMS CONFIGURE_DEPENDS user/programs/*.asm)
file(GLOB USER_C_PROGRAMS CONFIGURE_DEPENDS user/programs/*.c)

set(USER_PROGRAM_TARGETS "")

# Build assembly userland programs
foreach(user_asm ${USER_ASM_PROGRAMS})
    get_filename_component(prog_name ${user_asm} NAME_WE)
    set(user_obj "${CMAKE_BINARY_DIR}/user/${prog_name}.o")
    set(user_elf "${CMAKE_BINARY_DIR}/user/${prog_name}.elf")
    
    add_custom_command(
        OUTPUT ${user_obj}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/user
        COMMAND nasm -f elf32 -o ${user_obj} ${user_asm}
        DEPENDS ${user_asm}
        COMMENT "Building userland program: ${prog_name}.asm"
    )
    
    add_custom_command(
        OUTPUT ${user_elf}
        COMMAND ld -m elf_i386 -Ttext 0x00400000 -e _start -o ${user_elf} ${user_obj}
        DEPENDS ${user_obj}
        COMMENT "Linking userland program: ${prog_name}.elf"
    )
    
    list(APPEND USER_PROGRAM_TARGETS ${user_elf})
endforeach()

# Build C programs (if compiler available)
find_program(I686_COMPILER NAMES i686-elf-gcc i686-linux-gnu-gcc gcc)
if(I686_COMPILER)
    message(STATUS "Found cross compiler: ${I686_COMPILER}")
    
    # First, compile the graphics library
    file(GLOB USER_LIB_SOURCES CONFIGURE_DEPENDS user/lib/*.c)
    set(USER_LIB_OBJECTS "")
    
    foreach(lib_c ${USER_LIB_SOURCES})
        get_filename_component(lib_name ${lib_c} NAME_WE)
        set(lib_obj "${CMAKE_BINARY_DIR}/user/lib/${lib_name}.o")
        
        add_custom_command(
            OUTPUT ${lib_obj}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/user/lib
            COMMAND ${I686_COMPILER} -m32 -ffreestanding -nostdlib 
                -I${CMAKE_SOURCE_DIR}/user/include -c -o ${lib_obj} ${lib_c}
            DEPENDS ${lib_c}
            COMMENT "Compiling userland library: ${lib_name}.c"
        )
        
        list(APPEND USER_LIB_OBJECTS ${lib_obj})
    endforeach()
    
    # Now build C programs and link with library
    foreach(user_c ${USER_C_PROGRAMS})
        get_filename_component(prog_name ${user_c} NAME_WE)
        set(user_elf "${CMAKE_BINARY_DIR}/user/${prog_name}.elf")
        
        add_custom_command(
            OUTPUT ${user_elf}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/user
            COMMAND ${I686_COMPILER} -m32 -ffreestanding -nostdlib -nostartfiles 
                -I${CMAKE_SOURCE_DIR}/user/include -Ttext=0x00400000 
                -o ${user_elf} ${user_c} ${USER_LIB_OBJECTS}
            DEPENDS ${user_c} ${USER_LIB_OBJECTS}
            COMMENT "Building userland C program: ${prog_name}.c"
        )
        
        list(APPEND USER_PROGRAM_TARGETS ${user_elf})
    endforeach()
else()
    message(STATUS "No suitable compiler found, skipping C userland programs")
endif()

# Create a target for all userland programs
add_custom_target(userland ALL DEPENDS ${USER_PROGRAM_TARGETS})

# Copy userland programs to FAT16 filesystem
add_custom_target(install_programs
    COMMAND ${CMAKE_COMMAND} -E echo "Mounting FAT16 filesystem..."
    COMMAND umount ${CMAKE_SOURCE_DIR}/fat16 || true
    COMMAND mount -t vfat ${CMAKE_SOURCE_DIR}/fs.img ${CMAKE_SOURCE_DIR}/fat16 -o shortname=win95 || true
    COMMAND ${CMAKE_COMMAND} -E echo "Copying userland programs to fat16/..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/user ${CMAKE_SOURCE_DIR}/fat16
    COMMAND ${CMAKE_COMMAND} -E echo "Done! Programs installed to fat16/"
    DEPENDS userland
    COMMENT "Installing userland programs to FAT16 filesystem"
)

# Clear the FAT16 filesystem and build cache
add_custom_target(cleardrive
    COMMAND ${CMAKE_COMMAND} -E echo "Clearing build/user cache..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/user
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/user
    COMMAND ${CMAKE_COMMAND} -E echo "Mounting FAT16 filesystem..."
    COMMAND umount ${CMAKE_SOURCE_DIR}/fat16 || true
    COMMAND mount -t vfat ${CMAKE_SOURCE_DIR}/fs.img ${CMAKE_SOURCE_DIR}/fat16 -o shortname=win95 || true
    COMMAND ${CMAKE_COMMAND} -E echo "Clearing all files from fat16/..."
    COMMAND find ${CMAKE_SOURCE_DIR}/fat16 -type f -delete || true
    COMMAND ${CMAKE_COMMAND} -E echo "Unmounting filesystem..."
    COMMAND umount ${CMAKE_SOURCE_DIR}/fat16 || true
    COMMAND ${CMAKE_COMMAND} -E echo "Done! FAT16 filesystem and build cache cleared."
    COMMENT "Clearing FAT16 filesystem and build cache"
)

# --- Optional: Initiate QEMU for testing ---
add_custom_target(run
    COMMAND qemu-system-i386 -smp 4 -cpu pentium3 -boot d -cdrom my-kernel.iso -drive file=../fs.img,format=raw,index=0,media=disk -m 4G -d int -D ../qemu.log
    DEPENDS kernel
    COMMENT "Starting QEMU with the built ISO"
)

add_custom_target(debug
    COMMAND qemu-system-i386 -smp 4 -cpu pentium3 -boot d -cdrom my-kernel.iso -drive file=../fs.img,format=raw,index=0,media=disk -m 4G -s -S
    DEPENDS kernel
    COMMENT "Starting QEMU in debug mode (listening on port 1234)"
)

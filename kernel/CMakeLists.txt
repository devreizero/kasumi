cmake_minimum_required(VERSION 3.20)
project(KasumiKernel LANGUAGES C)

# ===================================================================================================================================================
# Validating Config
# ===================================================================================================================================================

set(SUPPORTED_ARCHS x64)
set(SUPPORTED_BOOTLOADERS limine)

set(ARCH "x64" CACHE STRING "Target architecture")
set(BOOTLOADER "limine" CACHE STRING "Boot protocol")

if (NOT ARCH IN_LIST SUPPORTED_ARCHS)
    message(FATAL_ERROR "Unsupported ARCH: ${ARCH}. Supported: ${SUPPORTED_ARCHS}")
endif()

if (NOT BOOTLOADER IN_LIST SUPPORTED_BOOTLOADERS)
    message(FATAL_ERROR "Unsupported BOOTLOADER: ${BOOTLOADER}. Supported: ${SUPPORTED_BOOTLOADERS}")
endif()

set(TARGET_NAME "kasumi-${ARCH}-${BOOTLOADER}.bin")

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_LINKER ld.lld)

# ===================================================================================================================================================
# Setting Output Directory
# ===================================================================================================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# ===================================================================================================================================================
# Architecture Setup
# ===================================================================================================================================================

if(ARCH STREQUAL "x64")
    set(CMAKE_SYSTEM_PROCESSOR "x86_64")
    set(ARCH_DIR "x86")
    set(ARCH_FLAGS "-m64")
    add_definitions(-DARCH_64)
elseif(ARCH STREQUAL "arm64")
    set(CMAKE_SYSTEM_PROCESSOR "aarch64")
    set(ARCH_DIR "arm64")
    set(ARCH_FLAGS "-march=armv8-a")
    add_definitions(-DARCH_64)
elseif(ARCH STREQUAL "arm32")
    set(CMAKE_SYSTEM_PROCESSOR "arm")
    set(ARCH_DIR "arm")
    set(ARCH_FLAGS "-marm")
    add_definitions(-DARCH_32)
endif()

# ===================================================================================================================================================
# Executable Configuration
# ===================================================================================================================================================

add_executable(${TARGET_NAME})

file(GLOB_RECURSE KERNEL_SOURCES
    src/common/*.c
    src/common/**/*.c
    src/libc/*.c
    src/libc/*.S
    src/arch/${ARCH_DIR}/*.c
    src/arch/${ARCH_DIR}/*.S
    src/arch/${ARCH_DIR}/**/*.c
    src/arch/${ARCH_DIR}/**/*.S
    src/protocol/${PROT_DIR}/*.c
    src/protocol/${PROT_DIR}/*.S
    src/protocol/${PROT_DIR}/**/*.c
    src/protocol/${PROT_DIR}/**/*.S
)

target_sources(${TARGET_NAME} PRIVATE ${KERNEL_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    include/common
    include/arch
    include/protocol
    include/libc
)

target_compile_options(${TARGET_NAME} PRIVATE
    -ffreestanding
    -fno-stack-protector
    -fno-pic
    -fno-pie
    -O0
    -nostdlib
    -nostdinc
    -Wall
    -Wextra
    -mcmodel=kernel
    ${ARCH_FLAGS}
)

# ===================================================================================================================================================
# Linker Script-related Variables
# ===================================================================================================================================================

set(ENTRY_SYMBOL "kmain")

if(ARCH STREQUAL "x64")
    set(LOAD_ADDRESS "0x100000")
    set(TARGET_OUTPUT_FORMAT "elf64-x86-64")
elseif(ARCH STREQUAL "arm64")
    set(LOAD_ADDRESS "0x80000")
elseif(ARCH STREQUAL "arm32")
    set(LOAD_ADDRESS "0x8000")
endif()

if(BOOTLOADER STREQUAL "limine")
    set(LOAD_ADDRESS "0xffffffff80000000")

    set(PROTOCOL_PREAMBLE [[
/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions; this also allows us to exert more control over the linking */
/* process. */
PHDRS
{
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}
]])

    set(PROTOCOL_EARLY_SECTIONS [[
    /* Define a section to contain the Limine requests and assign it to its own PHDR */
    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } > LIMINE

    . = ALIGN(CONSTANT(MAXPAGESIZE));
]])

elseif(BOOTLOADER STREQUAL "grub2")

    set(PROTOCOL_FINAL_SECTIONS [[
    .multiboot : {
        KEEP(*(.multiboot*))
    }
]])

endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/linker.ld.in
    ${CMAKE_BINARY_DIR}/linker.ld
)

# ===================================================================================================================================================
# Linker Script Configuration
# ===================================================================================================================================================

target_link_options(${TARGET_NAME} PRIVATE
    "-T${CMAKE_BINARY_DIR}/linker.ld"
    --no-pie
    -gc-sections
    -nostdlib
    -static
)

# ===================================================================================================================================================
# Output Config
# ===================================================================================================================================================

set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    OUTPUT_NAME ${TARGET_NAME}
)
set_target_properties(${TARGET_NAME} PROPERTIES
    LINKER_LANGUAGE C
    LINK_COMMAND "${CMAKE_LINKER} <LINK_FLAGS> <OBJECTS> -o <TARGET>"
)

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

message(STATUS "Kernel build target: ${TARGET_NAME}")
message(STATUS "Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}")

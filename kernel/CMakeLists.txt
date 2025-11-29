cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
project(KasumiKernel LANGUAGES C)

# ============================================================
# Configurable Build Parameters
# ============================================================

set(SUPPORTED_ARCHS x64 arm64 arm32)
set(SUPPORTED_BOOTLOADERS limine grub2)

set(ARCH "x64" CACHE STRING "Target architecture")
set(BOOTLOADER "limine" CACHE STRING "Boot protocol")

if (NOT ARCH IN_LIST SUPPORTED_ARCHS)
    message(FATAL_ERROR "Unsupported ARCH: ${ARCH}. Supported: ${SUPPORTED_ARCHS}")
endif()

if (NOT BOOTLOADER IN_LIST SUPPORTED_BOOTLOADERS)
    message(FATAL_ERROR "Unsupported BOOTLOADER: ${BOOTLOADER}. Supported: ${SUPPORTED_BOOTLOADERS}")
endif()

set(TARGET_NAME "build-kernel-${ARCH}-${BOOTLOADER}")

# ============================================================
# Output directory
# ============================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# ============================================================
# Architecture setup
# ============================================================

if(ARCH STREQUAL "x64")
    set(ARCH_DIR "x86")
    set(CMAKE_SYSTEM_PROCESSOR "x86_64")
    set(ARCH_FLAGS "-m64")
    set(TARGET_OUTPUT_FORMAT "elf64-x86-64")
elseif(ARCH STREQUAL "arm64")
    set(ARCH_DIR "arm64")
    set(CMAKE_SYSTEM_PROCESSOR "aarch64")
    set(ARCH_FLAGS "-march=armv8-a")
elseif(ARCH STREQUAL "arm32")
    set(ARCH_DIR "arm")
    set(CMAKE_SYSTEM_PROCESSOR "arm")
    set(ARCH_FLAGS "-marm")
endif()

# ============================================================
# Bootloader setup
# ============================================================

set(PROT_DIR "${BOOTLOADER}")

# ============================================================
# Linker script configuration
# ============================================================

if(ARCH STREQUAL "x64")
    set(LOAD_ADDRESS "0x100000")
    set(ENTRY_SYMBOL "kmain")
elseif(ARCH STREQUAL "arm64")
    set(LOAD_ADDRESS "0x80000")
    set(ENTRY_SYMBOL "kmain")
elseif(ARCH STREQUAL "arm32")
    set(LOAD_ADDRESS "0x8000")
    set(ENTRY_SYMBOL "kmain")
endif()

# ------------------------------------------------------------
# Protocol-specific linker sections
# ------------------------------------------------------------

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
} :limine_requests
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

# ============================================================
# Compiler Flags
# ============================================================

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -fno-stack-protector -nostdlib -fno-pic -fno-pie ${ARCH_FLAGS} -Wall -Wextra -O2")

# ============================================================
# Source Discovery
# ============================================================

file(GLOB_RECURSE KERNEL_SOURCES
    src/common/*.c
    src/arch/${ARCH_DIR}/*.c
    src/arch/${ARCH_DIR}/*.S
    src/protocol/${PROT_DIR}/*.c
    src/protocol/${PROT_DIR}/*.S
)

# ============================================================
# Target Definition
# ============================================================

add_executable(${TARGET_NAME} ${KERNEL_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    include/common
    include/arch
    include/protocol
)

target_link_options(${TARGET_NAME} PRIVATE
    "-T${CMAKE_BINARY_DIR}/linker.ld"
    "-ffreestanding"
    "-fno-pic"
    "-fno-pie"
    "-nostdlib"
    "-no-pie"
)

# ============================================================
# Output Config
# ============================================================

set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    OUTPUT_NAME ${TARGET_NAME}
)

message(STATUS "Kernel build target: ${TARGET_NAME}")
message(STATUS "Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}")

#!/usr/bin/env bash
set -euo pipefail

# ============================================================
# Build Limine ISO for Kasumi Kernel
# ============================================================

# Usage: ./build-limine-iso.sh [ARCH]
ARCH="${1:-x64}"
ISO_NAME="kasumi-limine-${ARCH}.iso"

# Directories
SCRIPT_DIR=$(dirname "$(realpath "$0")")
KERNEL_ROOT=$(realpath "$SCRIPT_DIR/..")
PROJECT_ROOT=$(realpath "$KERNEL_ROOT/..")
BUILD_DIR="$KERNEL_ROOT/build"
ISO_ROOT="$BUILD_DIR/iso_root"

KERNEL_BIN="$BUILD_DIR/build/kasumi-${ARCH}-limine.bin"
LIMINE_BOOT="$PROJECT_ROOT/tools/limine/boot"
LIMINE_CFG="$PROJECT_ROOT/tools/limine/limine.conf"

# Clean previous ISO root
rm -rf "$ISO_ROOT"
mkdir -p "$ISO_ROOT/boot/limine"

# Copy Limine binaries
cp -rf "$LIMINE_BOOT"/* "$ISO_ROOT/boot/limine/"

# Move EFI directory
mkdir -p "$ISO_ROOT/EFI"
mv "$ISO_ROOT/boot/limine/EFI" "$ISO_ROOT/EFI/BOOT"

# Copy kernel
cp "$KERNEL_BIN" "$ISO_ROOT/boot/kasumi"

# Copy limine.cfg
cp "$LIMINE_CFG" "$ISO_ROOT/boot/limine"

# Generate ISO using xorriso
xorriso -as mkisofs                             \
    -R -r -J                                    \
    -b boot/limine/limine-bios-cd.bin           \
    -no-emul-boot                               \
    -boot-load-size 4                           \
    -boot-info-table                            \
    --efi-boot boot/limine/limine-uefi-cd.bin   \
    -efi-boot-part                              \
    --efi-boot-image                            \
    --protective-msdos-label                    \
    -o "$BUILD_DIR/$ISO_NAME"                   \
    "$ISO_ROOT"

echo "✅ Limine ISO created: $BUILD_DIR/$ISO_NAME"

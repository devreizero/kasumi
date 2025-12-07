# Kasumi

**What is Kasumi?** To put it simply, it's AurealOS reborn. No.

## Getting Started
1. Build the OS
```sh
git clone https://github.com/devreizero/kasumi.git
cd kasumi/kernel

mkdir build                 # Yes, do it manually please
cmake -G Ninja -B build     # Not necesarrily must Ninja, but it's recommended
ninja -C build
```

2. Build the ISO
```sh
./scripts/build-limine-iso.sh
```

3. Run it with QEMU
```sh
qemu-system-x86_64 -cdrom build/kasumi-limine-x64.iso
```
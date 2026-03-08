---
description: How to build the project on Linux using CMake
---

# Linux native CMake Build Workflow

This workflow details how to build the server natively on a Linux environment (e.g., Ubuntu/Debian) without Docker.

## Prerequisites
- GCC/G++ or Clang compiler installed (C++20/C++23 support required).
- CMake, Make/Ninja, and required system dependencies (libboost, libssl, libfmt, libpugixml, etc., or handled via vcpkg).
- The `recompile.sh` or `build_test.sh` scripts are available.

## Steps

1. **Clean previous builds (Optional but recommended)**
```bash
rm -rf build
```

2. **Create build directory and configure**
```bash
mkdir build && cd build
cmake ..
```

3. **Compile the server**
   Use all available processor cores for a faster build.
```bash
make -j$(nproc)
```

4. **Alternative: Using the shell script**
   The repository contains a `recompile.sh` helper script.
```bash
// turbo
sh recompile.sh
```

5. **Start the server**
   Once compiled, the binary `canary` will be generated. Run it:
```bash
./canary
```

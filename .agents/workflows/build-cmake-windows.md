---
description: How to build the project on Windows using CMake and vcpkg
---

# Windows CMake & vcpkg Build Workflow

This workflow details how an agent should build the Kaldmor Canary OT server natively on Windows using MSVC, CMake, and vcpkg.

## Prerequisites
- Visual Studio 2022 (or Build Tools) installed.
- CMake installed and available in standard paths.
- vcpkg installed locally or initialized as a submodule. (The project usually downloads vcpkg automatically during the configure step via CMake or build scripts).

## Steps

1. **Configure the Project**
   Use CMake with the predefined `windows-release` preset (found in `CMakePresets.json`).
```powershell
// turbo
cmake --preset windows-release
```

2. **Build the Project**
   Compile the server using the configured directory. Adjust concurrency if needed.
```powershell
// turbo
cmake --build build/windows-release --config Release -j 8
```

3. **Locate the Executable**
   The compiled executable will typically be placed in the root directory (e.g., `canary.exe`) or inside the `build/windows-release/` folder depending on the `CMakeLists.txt` output configuration.

4. **Troubleshooting**
   If vcpkg fails to find a dependency, ensure the vcpkg baseline is up to date, or clear the local vcpkg cache and rerun the configuration step.

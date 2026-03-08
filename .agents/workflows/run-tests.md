---
description: How to run C++ and Lua unit tests
---

# Testing Workflow

This workflow details how to run the project's tests locally before submitting a Pull Request.

## Prerequisites
- The project must be built or configured to build unit tests (`ENABLE_TESTING=ON` in CMake).
- For Lua tests, Busted might be required or the integrated Canary test framework.

## Steps

1. **Run C++ Unit Tests (Linux/WSL)**
   The `build_test.sh` script automates the build and run process for tests.
```bash
// turbo
sh build_test.sh
```

2. **Run C++ Unit Tests (Windows/CMake)**
   If using CMake on Windows, configure the build with testing enabled, build the test target (usually `CanaryTests.exe` or similar depending on the `CMakeLists` configuration), and run CTest.
```powershell
cmake --preset windows-release
cmake --build build/windows-release --target tests
ctest --test-dir build/windows-release -C Release --output-on-failure
```

3. **Review Output**
   Ensure all tests pass. If a test fails, do not proceed with a PR until the failure is resolved. It's often related to memory leaks, unhandled exceptions, or incorrect datapack loading.

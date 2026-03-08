---
description: Kaldmor Canary OT Project Rules
---

# Kaldmor Canary OT Rules

This workspace follows specific guidelines to ensure code quality, maintainability, and consistency with the OpenTibia ecosystem, specifically the Canary engine. All AI agents (Antigravity, Cursor, Windsurf, etc.) must adhere to these rules when interacting with this repository.

**CRITICAL CONTEXT:** This development environment is running on **Windows**, but the primary goal is to **always compile and run the server on Linux via Docker**. While local Windows compilation might occur for intellisense, you must strictly prioritize Linux/Docker compatibility for all builds, code modifications, and tests.

## 1. C++ Core Development (src/)
- **Linux/Docker First:** All C++ code changes must compile flawlessly in the project's Linux Docker container (`docker compose up --build`). Look out for Windows-specific headers or paths; do not use them.
- **Modern C++:** Utilize modern C++ features (C++20/23) taking advantage of the project's setup.
- **Memory Management:** Strongly prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers to prevent memory leaks.
- **CMake:** Whenever a new `.cpp` or `.hpp` file is created, or an existing one is deleted, YOU MUST update the corresponding `CMakeLists.txt` (typically `src/CMakeLists.txt`).
- **Includes:** Keep includes minimal and use forward declarations where possible in headers to reduce compilation times.
- **Formatting:** Adhere to the project's `.clang-format` rules.

## 2. LUA Scripting (data/)
- **RevScriptSys:** Always use the latest Canary "RevScriptSys" format for new scripts. Do not use the legacy XML-based event registration unless explicitly necessary or dealing with legacy systems that haven't been ported.
- **Event Registration:** Register events directly within the Lua script files.
- **Formatting:** Use consistent indentation (check existing scripts for the prevailing style, typically tabs or 4 spaces as per the project's standard). Use clear and descriptive variable names.

## 3. Database & SQL
- **Schema Changes:** Any new tables or modifications to existing tables must be reflected in `schema.sql`.
- **Security:** PREVENT SQL INJECTION. Always use Prepared Statements or the appropriate escaping mechanisms provided by the database abstraction layer when constructing queries with user input or variables.

## 4. GitHub Flow & CI/CD
- **Granular PRs:** Create small, focused branches. A Pull Request should address a single logical change or feature.
- **PR Templates:** Always populate Pull Requests using the provided `.github/PULL_REQUEST_TEMPLATE.md`.
- **Validation:** Before proposing complex changes, consider the CI pipeline. Ensure code passes linters (`cppcheck`), security scans (`CodeQL`), and builds successfully across platforms (as defined in the GitHub Actions workflows).

## 5. Agent Instructions
- Try to read and understand existing code patterns in the repository before writing new implementations.
- If unsure about a specific Canary engine mechanic, search the repository for similar implementations.
- Always communicate clearly about what files are being modified and why.

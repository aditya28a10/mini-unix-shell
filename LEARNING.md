# Mini Unix Shell — Developer Learning Companion (`LEARNING.md`)

This document is dedicated to your personal mastery of Operating Systems, Systems Programming, and Computer Architecture concepts throughout the development of `mini-shell`.

Rather than treating the codebase as an opaque set of instructions, use this document to track the fundamental *why*, prepare for technical interviews, and bridge theory with actual POSIX implementation.

---

## Phase 0: Toolchain, Build Systems, and the POSIX Interface

### 1. Concepts You Should Understand

- **The C/C++ Translation Pipeline**:
  - **Preprocessing (`cpp`)**: Expands `#include` directives, macros (`#define`), and conditional compilation (`#ifdef`). Generates translation units (`.ii` / `.i`).
  - **Compilation (`cc1plus`)**: Translates high-level C++ AST (Abstract Syntax Tree) into assembly code (`.s`) specific to the target architecture (x86_64, ARM64/Apple Silicon).
  - **Assembly (`as`)**: Converts assembly mnemonics into machine code stored within relocatable object files (`.o` / ELF on Linux, Mach-O on macOS).
  - **Linking (`ld`)**: Resolves external symbols across object files and libraries (e.g., standard C/C++ runtime `libc`/`libstdc++`), assigning final virtual memory addresses to create an executable binary.
- **Header Files vs. Source Files**:
  - Why `#ifndef` / `#pragma once` header guards exist: preventing duplicate symbol definitions during the single translation unit compilation phase (the One Definition Rule - ODR).
  - Difference between declaration (`extern`, function signatures, class layouts) and definition (allocating memory or emitting machine instructions).
- **Makefiles & Dependency Tracking**:
  - Make targets, prerequisites, and recipes: `target: prerequisites \n \t recipe`.
  - Why timestamps matter: `make` checks if the prerequisite's `mtime` is newer than the target's `mtime` to avoid redundant compilation.
  - Dependency generation (`-MMD -MP`): Instructs the compiler to output `.d` files tracking which header files each `.cpp` depends upon, ensuring rebuilds when headers change.
- **POSIX Standards**:
  - Portable Operating System Interface (POSIX): An IEEE standard family (IEEE 1003.1) defining the system call interface, C library API, and shell environment utilities across Unix-like kernels (Linux, FreeBSD, macOS/Darwin, Solaris).
  - Why write to POSIX APIs: Guarantees your shell logic is portable across Unix variants without rewrite.

---

### 2. Important Questions to Ask Yourself

1. *What is the difference between an executable file on disk and a process in memory?*
   - An executable is passive machine code and static data (ELF/Mach-O format) on non-volatile storage.
   - A process is an active, running instance of that program with allocated kernel state: virtual address space (text, data, heap, stack), registers (PC, SP), file descriptor table, and PID.
2. *Why do we compile with `-Wall -Wextra -Wpedantic`?*
   - C++ allows subtle undefined behaviors. Strict compiler flags convert hidden pitfalls (e.g., uninitialized variables, signed/unsigned comparison mismatches, dangling references) into immediate compiler warnings.
3. *Why do we use standard exit codes (`0` vs. non-zero)?*
   - In Unix convention, `0` indicates success (`EXIT_SUCCESS`). Any non-zero value (typically `1` to `255`) represents a specific error category or termination signal (`EXIT_FAILURE`), allowing calling processes (or parent shells) to inspect `$?`.

---

### 3. Common Misconceptions

- **Misconception**: *"`g++` is the compiler."*
  - **Reality**: `g++` (or `clang++`) is a *compiler driver*. It invokes the preprocessor, the actual compiler (`cc1plus`), the assembler (`as`), and the linker (`ld`) in sequence.
- **Misconception**: *"`#include <iostream>` copies the library code into my file."*
  - **Reality**: It only copies the *declarations* and templates into your translation unit. The actual compiled machine code lives in `libstdc++.so` / `libc++.dylib` and is linked by `ld`.
- **Misconception**: *"A shell is part of the operating system kernel."*
  - **Reality**: A shell is purely a user-space application! It interacts with the kernel strictly through system calls (`fork`, `execve`, `read`, `write`). You can write a complete shell without writing any kernel code.

---

### 4. System Calls to Know in Phase 0

While Phase 0 is primarily scaffolding, here are the low-level POSIX and C library primitives touched during program startup and termination:

| Primitive | Category | Purpose in Unix / C++ |
| :--- | :--- | :--- |
| `write(2)` / `std::cout` | I/O System Call | Writes byte streams to standard output (file descriptor `1`). |
| `exit(3)` / `return` from `main` | Process Termination | Flushes user-space I/O buffers, executes global destructors, and invokes `exit_group(2)` / `_exit(2)`. |
| `_exit(2)` | Kernel System Call | Immediately terminates process without running user-space cleanups (critical after failed `fork`). |

---

### 5. Questions an Interviewer Might Ask

1. **"Walk me through what happens under the hood when you run `make` on a C++ project."**
   - *Key points*: Evaluation of dependency DAG (Directed Acyclic Graph), timestamp comparisons, execution of subshells for dirty targets, compilation to `.o`, linkage to binary.
2. **"What is the difference between static linking and dynamic linking?"**
   - *Static*: Code from libraries is physically copied into the executable binary at link time. Results in larger binaries, but self-contained with no runtime dependency.
   - *Dynamic*: The linker leaves stub references. At runtime, the OS dynamic loader (`ld.so` / `dyld`) maps shared libraries (`.so` / `.dylib`) into the process's virtual memory space.
3. **"Why should systems code avoid `using namespace std;` in header files?"**
   - *Key points*: Namespace pollution. Headers are transitively included across translation units; bringing all symbols from `std` into global scope can cause name collisions and ambiguous overload resolutions.

---

### 6. Things Requiring Deeper OS / Computer Architecture Study

- **Memory layout of an ELF/Mach-O binary**:
  - Text segment (code, read-only, executable).
  - Data segment (initialized global/static variables).
  - BSS segment (uninitialized global/static variables, zero-filled at launch).
  - Heap (dynamically allocated memory growing upward).
  - Stack (stack frames, local variables, return addresses, growing downward).
- **Virtual Memory & Address Translation**:
  - How the MMU (Memory Management Unit) and Page Tables map process virtual addresses to physical RAM frames.
  - Why separate processes cannot corrupt each other's memory.

---

### 7. Phase 0 Completion Checklist

- [x] Directory structure matches Unix conventions (`src/`, `include/`, `tests/`, `docs/`).
- [x] Makefile with incremental dependency tracking (`-MMD -MP`) and clean separation of `obj/` and `bin/`.
- [x] Executable produces correct exit codes and handles CLI flags (`--help`, `--version`).
- [x] Automated test harness confirms build and invocation integrity.
- [x] No Phase 1 functionality prematurely introduced.

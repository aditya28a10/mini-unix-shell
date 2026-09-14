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

---

## Phase 1: Process Creation, Execution, and Synchronization

### 1. Concepts You Should Understand

- **Process vs. Program**:
  - A *program* is a passive entity: bytes stored in an executable file on secondary storage formatted as ELF (Linux) or Mach-O (macOS).
  - A *process* is an active operating system abstraction: a program loaded into virtual memory with dedicated hardware registers (Program Counter, Stack Pointer), an address space (text, data, heap, stack), an open file descriptor table, credentials (UID/GID), and a Process ID (PID).
- **The Process Control Block (PCB)**:
  - The kernel structure (`struct task_struct` in Linux) that tracks process state: PID, parent PID (PPID), CPU state, scheduling priority, memory descriptors (`mm_struct`), and file descriptor table (`files_struct`).
- **Process Hierarchy & Fork Semantics**:
  - In traditional Unix environments and standard shell architecture, `fork()` is the classic mechanism used to create new user-space processes. In this shell, process creation relies explicitly on `fork()` to spawn child execution contexts.
  - `fork()` clones the caller. The kernel creates a new PCB, allocates a new PID, and duplicates the memory page tables of the parent.
  - **Copy-on-Write (COW)**: The kernel does not copy all physical memory pages at fork time! Instead, parent and child page tables point to the *same physical frames*, marked read-only. Only when either process writes to a page does a Page Fault trigger, prompting the kernel to allocate a distinct physical frame.
- **Address Space Overwriting (`execvp`)**:
  - Unlike `fork()`, which creates a new process, `exec()` does not create a process. It discards the existing virtual address space (text, data, heap, stack) and loads a new executable into the *same process identity (PID stays identical)*.
  - Open file descriptors remain open across `exec()` unless configured with `FD_CLOEXEC` (Close-On-Exec).
- **Parent Synchronization & Zombie Processes (`waitpid`)**:
  - When a child process calls `exit()`, it terminates its execution, releases its virtual address space, and closes its file descriptors.
  - However, its PCB and exit status remain in the kernel table so the parent can read what happened. During this window, the child is a **zombie** (`defunct`).
  - If the parent never calls `wait()` or `waitpid()`, zombie processes accumulate and exhaust available PIDs in the OS PID table.
  - Calling `waitpid()` reaps the zombie and frees the PCB.
- **Exit Status and Inspection Macros**:
  - POSIX specifies that the raw integer status filled by `waitpid(&status, ...)` must not be inspected or decoded directly with arbitrary bitwise operations, as its internal bit layout is implementation-defined across Unix platforms.
  - Instead, POSIX provides standardized status macros to inspect the child's termination state in an implementation-independent manner:
    - `WIFEXITED(status)`: Evaluates to non-zero (true) if the child process terminated normally (e.g., called `exit()`, `_exit()`, or returned from `main()`).
    - `WEXITSTATUS(status)`: If `WIFEXITED(status)` is true, evaluates to the low-order 8 bits of the status argument passed to `exit()`, `_exit()`, or returned from `main()`.
    - `WIFSIGNALED(status)`: Evaluates to non-zero (true) if the child process was terminated by an unhandled signal (e.g., `SIGINT`, `SIGKILL`, `SIGSEGV`).
    - `WTERMSIG(status)`: If `WIFSIGNALED(status)` is true, evaluates to the signal number that caused the child process to terminate.

---

### 2. Important Questions to Ask Yourself

1. *Why does `fork()` return twice?*
   - Because `fork()` duplicates the execution context. When the system call finishes inside the kernel, the hardware state (registers, instruction pointer) for two distinct processes is set to return from the system call.
   - To distinguish the two, the kernel writes `0` into the return register (`%rax`/`x0`) of the child process, and writes the child's PID into the return register of the parent process.
2. *What happens when `execvp()` fails?*
   - Because the process address space is only overwritten if the target binary is valid and loaded, a failed `execvp()` returns control back to the instruction immediately following it in the child process.
   - If you don't handle this return, the child process would continue executing the shell's code, leading to two duplicate shell loops reading from the same stdin terminal!
   - Hence, a failed `execvp()` must report the error and terminate immediately using `_exit()`.
3. *Why should a child use `_exit()` instead of `exit()` on exec failure?*
   - Standard `exit(3)` flushes user-space stdio stream buffers and invokes C++ static object destructors. If child and parent share stream buffers from before the fork, calling `exit(3)` in the child flushes the parent's data twice or invokes destructors on shared resources. `_exit(2)` directly invokes the kernel syscall to terminate without touching user-space buffers.

---

### 3. Common Misconceptions

- **Misconception**: *"A new process gets a copy of all RAM from its parent."*
  - **Reality**: Modern OS kernels use Copy-on-Write (COW). Memory pages are shared read-only until written to. Since `fork()` in a shell is almost immediately followed by `exec()`, copying memory would be massive wasted overhead.
- **Misconception**: *"`execvp` creates a new process."*
  - **Reality**: `execvp` completely replaces the running program inside the *existing* process. The PID does not change.
- **Misconception**: *"When a process calls `exit(0)`, it is completely gone from the kernel."*
  - **Reality**: It becomes a zombie until its parent calls `wait` or `waitpid`. Only when reaped is its entry removed from the process table.

---

### 4. System Calls to Know in Phase 1

| System Call | Header | Purpose in `mini-shell` |
| :--- | :--- | :--- |
| `fork(2)` | `<unistd.h>` | Clones the shell process to create a child execution context. |
| `execvp(3)` | `<unistd.h>` | Replaces the child process image with the specified command, searching `$PATH`. |
| `waitpid(2)` | `<sys/wait.h>` | Synchronously suspends parent shell until child terminates; reaps child status. |
| `_exit(2)` | `<unistd.h>` | Cleanly terminates the child process immediately if `execvp` fails. |
| `isatty(3)` | `<unistd.h>` | Checks if file descriptor 0 is a terminal (for prompt rendering). |

---

### 5. Questions an Interviewer Might Ask

1. **"What is the Fork-Exec model, and why did Unix choose it over a single `spawn()` system call (like Windows `CreateProcess`)?"**
   - *Key points*: Separation of concerns. By decoupling process creation (`fork`) from program execution (`exec`), the child process has a execution window in between to alter its environment (redirect stdin/stdout, change user credentials, close file descriptors, change process groups, alter signal masks) without cluttering `exec` with hundreds of configuration flags.
2. **"What is an orphan process, and how does Linux handle it?"**
   - *Key points*: If a parent terminates before its child, the child becomes an orphan. The kernel automatically re-parents the orphan to PID 1 (`systemd`/`init`), which periodically reaps terminated children.
3. **"What is a zombie process, why is it bad, and how do you prevent them?"**
   - *Key points*: A terminated process whose parent hasn't called `wait()`. While consuming zero CPU and RAM, it consumes a PID slot in the process table (`PID_MAX`). Prevent by synchronous waiting (`waitpid`) or asynchronous reaping via `SIGCHLD` signal handler (covered in Phase 7).

---

### 6. Things Requiring Deeper OS / Computer Architecture Study

- **Context Switching Overhead**:
  - What hardware operations occur when switching between parent and child: saving CPU registers (PC, SP, general-purpose), flushing or invalidating TLB (Translation Lookaside Buffer) caches unless ASID (Address Space Identifiers) are supported by the CPU MMU.
- **Kernel System Call Mechanics**:
  - How user-space enters kernel-space: `syscall` instruction (x86_64) or `svc` instruction (ARM64), transitioning from Ring 3 (User mode) to Ring 0 (Kernel mode), switching stacks from user stack to kernel stack.

---

### 7. Phase 1 Completion Checklist

- [x] Interactive prompt `mini-shell$ ` displayed when stdin is a TTY.
- [x] Input reading with `std::getline` and clean EOF (`Ctrl+D`) exit.
- [x] Whitespace-separated argument parser into `Command` struct.
- [x] Process creation using `fork()`.
- [x] Command execution using `execvp()` with `$PATH` resolution.
- [x] Immediate termination with `_exit()` if `execvp()` fails (codes 127/126).
- [x] Synchronous status collection via `waitpid()`.
- [x] Automated test suite verifying normal commands, exit codes, and error conditions.

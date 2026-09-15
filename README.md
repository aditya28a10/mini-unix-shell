# Mini Unix Shell (`mini-shell`)

An educational, undergraduate-level Unix shell implemented in C++ on Linux/POSIX. Designed from first principles to explore operating systems and computer architecture concepts: process creation and lifecycle, POSIX execution semantics, file descriptor redirection, pipelines, signal handling, and terminal job control.

---

## Overview

Modern command-line shells (such as `bash`, `zsh`, or `sh`) serve as the primary human interface to the operating system kernel. While they appear simple, a robust shell requires precise coordination of system calls, process groups, signals, and file descriptors.

The goal of **`mini-shell`** is not to wrap high-level abstractions or rely on convenience APIs like `system()` or `popen()`. Instead, all process creation, descriptor manipulation, pipeline wiring, and job state transitions are implemented directly using low-level POSIX/Linux system APIs.

---

## Architectural Roadmap

The project is developed incrementally across distinct phases:

| Phase | Milestone | Core Concepts / APIs | Status |
| :--- | :--- | :--- | :--- |
| **Phase 0** | Project Setup & Scaffolding | Toolchain, `Makefile`, strict compilation, unit test harness | Completed |
| **Phase 1** | **Basic Command Execution** | `fork()`, `execvp()`, `waitpid()`, REPL, exit statuses | **Completed** |
| **Phase 2** | **Built-in Commands** | In-process execution (`cd`, `pwd`, `exit`, `help`) | **Completed** |
| **Phase 3** | I/O Redirection | File descriptors, `open()`, `dup2()`, `close()`, stdin/stdout/stderr | Planned |
| **Phase 4** | Single Pipe (`\|`) | Inter-process communication, `pipe()`, descriptor plumbing | Planned |
| **Phase 5** | Multiple Pipelines | Multi-stage pipeline cascades, process chaining, resource cleanup | Planned |
| **Phase 6** | Background Execution (`&`) | Asynchronous jobs, non-blocking reaping, zombie prevention | Planned |
| **Phase 7** | Signal Handling | `sigaction()`, `SIGINT`, `SIGTSTP`, `SIGCHLD`, signal masks | Planned |
| **Phase 8** | Job Control | Process groups (`setpgid()`), terminal control (`tcsetpgrp()`), `jobs`, `fg`, `bg` | Planned |
| **Phase 9** | Robustness & Error Handling | Defensive programming, file leak prevention, error reporting | Planned |
| **Phase 10** | Polish & Comprehensive Test Suite | Automated regression testing, stress testing, edge-case coverage | Planned |

---

## Directory Structure

```text
mini-unix-shell/
├── include/           # Header files and interface declarations
│   ├── command.hpp    # Command data representation
│   ├── executor.hpp   # Process execution layer (fork, exec, waitpid)
│   ├── parser.hpp     # Whitespace-based tokenizer interface
│   ├── shell.hpp      # REPL coordinator and prompt management
│   ├── builtin.hpp    # Built-in command interface
│   └── version.hpp    # Version and build metadata
├── src/               # Implementation source files
│   ├── executor.cpp   # POSIX fork/exec/waitpid implementation
│   ├── main.cpp       # CLI entrypoint and options handling
│   ├── parser.cpp     # Whitespace tokenizer implementation
│   ├── shell.cpp      # REPL loop and built-in dispatch
│   └── builtin.cpp    # In-process built-in implementations
├── tests/             # Automated test scripts and harnesses
│   └── run_tests.sh   # Phase regression and compliance test runner
├── docs/              # In-depth architectural and phase documentation
│   ├── phase-00-setup.md
│   ├── phase-01-process-execution.md
│   └── phase-02-builtins.md
├── LEARNING.md        # Conceptual guide, OS questions, and study notes
├── README.md          # Public project overview and quickstart
├── Makefile           # POSIX build system
└── .gitignore         # Build and OS artifact exclusions
```

---

## Building and Running

### Prerequisites

- **Compiler**: C++17 compatible compiler (`g++` >= 9 or `clang++` >= 10)
- **Build Tool**: GNU Make (or POSIX-compatible `make`)
- **Platform**: Linux or POSIX-compliant operating system (e.g. macOS / Darwin)

### Build Targets

```bash
# Build the mini-shell binary
make

# Run the interactive shell
make run

# Run the automated test suite
make test

# Clean all build outputs (obj/, bin/)
make clean

# View all available targets
make help
```

---

## Usage Examples

Launch the shell interactively:

```text
$ ./bin/mini-shell
mini-shell$ ls
Makefile     README.md    bin          include      src
LEARNING.md  docs         obj          tests
mini-shell$ pwd
/Users/adityapal/Projects/mini-unix-shell
mini-shell$ echo hello from mini-shell
hello from mini-shell
mini-shell$ nonexistent_command
mini-shell: nonexistent_command: No such file or directory
mini-shell$
```

To exit the shell, press `Ctrl+D` (EOF).

---

## Testing

The project maintains an automated test suite located in `tests/`. Each phase introduces test assertions validating functionality, error conditions, and regressions.

Run the test suite using:

```bash
make test
```

---

## Current Status & Limitations

- **Current Phase**: Phase 2 (Built-in Commands).
- **Implemented**: Interactive REPL, whitespace parsing, `fork()`, `execvp()`, `waitpid()`, exit-status propagation, EOF handling, non-interactive input detection, and in-process `cd`, `pwd`, `exit`, and `help` built-ins.
- **Key design point**: Built-ins that modify shell state execute in the shell process; external commands retain the Phase 1 fork/exec/wait architecture.
- **Next Step**: Phase 3 — I/O redirection (`<`, `>`, `>>`) using file descriptors.

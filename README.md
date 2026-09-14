# Mini Unix Shell (`mini-shell`)

An educational, undergraduate-level Unix shell implemented in C++ on Linux/POSIX. Designed from first principles to explore operating systems and computer architecture concepts: process creation and lifecycle, POSIX execution semantics, file descriptor redirection, pipelines, signal handling, and terminal job control.

---

## Overview

Modern command-line shells (such as `bash`, `zsh`, or `sh`) serve as the primary human interface to the operating system kernel. While they appear simple, a robust shell requires precise coordination of system calls, process groups, signals, and file descriptors.

The goal of **`mini-shell`** is not to wrap high-level abstractions or rely on convenience APIs like `system()` or `popen()`. Instead, all process creation, descriptor manipulation, pipeline wiring, and job state transitions are implemented directly using low-level POSIX/Linux system APIs.

---

## Architectural Roadmap

The project is developed incrementally across distinct phases:

| Phase | Milestone | Core Concepts / APIs |
| :--- | :--- | :--- |
| **Phase 0** | **Project Setup & Scaffolding** *(Current)* | Toolchain, `Makefile`, strict compilation, unit test harness |
| **Phase 1** | Basic Command Execution | `fork()`, `execvp()`, `waitpid()`, process lifecycle |
| **Phase 2** | Built-in Commands | In-process execution (`cd`, `pwd`, `exit`, `help`) |
| **Phase 3** | I/O Redirection | File descriptors, `open()`, `dup2()`, `close()`, stdin/stdout/stderr |
| **Phase 4** | Single Pipe (`\|`) | Inter-process communication, `pipe()`, descriptor plumbing |
| **Phase 5** | Multiple Pipelines | Multi-stage pipeline cascades, process chaining, resource cleanup |
| **Phase 6** | Background Execution (`&`) | Asynchronous jobs, non-blocking reaping, zombie prevention |
| **Phase 7** | Signal Handling | `sigaction()`, `SIGINT`, `SIGTSTP`, `SIGCHLD`, signal masks |
| **Phase 8** | Job Control | Process groups (`setpgid()`), terminal control (`tcsetpgrp()`), `jobs`, `fg`, `bg` |
| **Phase 9** | Robustness & Error Handling | Defensive programming, file leak prevention, error reporting |
| **Phase 10** | Polish & Comprehensive Test Suite | Automated regression testing, stress testing, edge-case coverage |

---

## Directory Structure

```text
mini-unix-shell/
├── include/           # Header files and declarations
│   └── version.hpp    # Version and build metadata
├── src/               # Implementation source files
│   └── main.cpp       # Shell entrypoint
├── tests/             # Automated test scripts and harnesses
│   └── run_tests.sh   # Phase test runner
├── docs/              # In-depth architectural and phase documentation
│   └── phase-00-setup.md
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

# Run the executable
make run

# Run the automated test suite
make test

# Clean all build outputs (obj/, bin/)
make clean

# View all available targets
make help
```

---

## Testing

The project maintains an automated test suite located in `tests/`. Each phase introduces test assertions validating functionality, error conditions, and regressions.

Run the test suite using:

```bash
make test
```

---

## Current Status & Limitations

- **Current Phase**: Phase 0 (Project Setup & Toolchain Scaffolding).
- **Status**: Build environment, directory structure, compilation targets, and automated test harness are operational.
- **Next Step**: Phase 1 — implementing the interactive REPL, token parsing, and explicit `fork()` / `execvp()` / `waitpid()` process execution.

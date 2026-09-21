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
| **Phase 3** | **I/O Redirection** | File descriptors, `open()`, `dup2()`, `close()`, stdin/stdout/stderr | **Completed** |
| **Phase 4** | Single Pipe (`\|`) | Inter-process communication, `pipe()`, descriptor plumbing | **Completed** |
| **Phase 5** | Multiple Pipelines | Multi-stage pipeline cascades, process chaining, resource cleanup | **Completed** |
| **Phase 6** | Background Execution (`&`) | Asynchronous jobs, non-blocking reaping, zombie prevention | **Completed** |
| **Phase 7** | Signal Handling | `sigaction()`, `SIGINT`, `SIGCHLD`, `sig_atomic_t`, `EINTR` | **Completed** |
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

- **Current Phase**: Phase 7 (signal handling).
- **Implemented**: Interactive REPL, whitespace parsing, `fork()`, `execvp()`, `waitpid()`, exit-status propagation, EOF handling, non-interactive input detection, in-process `cd`, `pwd`, `exit`, and `help` built-ins, `<`, `>`, and `>>` I/O redirection using POSIX file descriptors, single/multiple-pipeline inter-process communication using `pipe()`, background command execution using `&`, and non-blocking background-process reaping using `waitpid(..., WNOHANG)`, POSIX signal handling using `sigaction()`, `SIGINT` and `SIGCHLD` handling, `sig_atomic_t` signal flags, child `SIGINT` restoration using `SIG_DFL`, and `EINTR` handling for foreground waits.

- **Key design point**: Built-ins that modify shell state execute in the shell process; external commands retain the fork/exec/wait architecture. Pipelines create `N - 1` pipes for `N` commands, with each child using `dup2()` to connect its input and output to the appropriate pipe ends. Background commands return immediately without a blocking `waitpid()`, while the shell uses `waitpid(..., WNOHANG)` to reap completed background children and prevent zombies. Signal handlers perform only minimal work by setting `sig_atomic_t` flags; the normal shell loop performs the actual signal handling work. Foreground `waitpid()` calls retry when interrupted with `EINTR`.

- **Next Step**: Phase 8 — (process groups and terminal job control).

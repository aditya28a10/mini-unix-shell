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
| **Phase 4** | **Single Pipe (`\|`)** | Inter-process communication, `pipe()`, descriptor plumbing | **Completed** |
| **Phase 5** | **Multiple Pipelines** | Multi-stage pipeline cascades, process chaining, resource cleanup | **Completed** |
| **Phase 6** | **Background Execution (`&`)** | Asynchronous jobs, non-blocking reaping, zombie prevention | **Completed** |
| **Phase 7** | **Signal Handling** | `sigaction()`, `SIGINT`, `SIGCHLD`, `sig_atomic_t`, `EINTR` | **Completed** |
| **Phase 8** | **Job Control** | Process groups (`setpgid()`), terminal control (`tcsetpgrp()`), `jobs`, `fg`, `bg` | **Completed** |
| **Phase 9** | Robustness & Error Handling | Defensive programming, file leak prevention, error reporting | Planned |
| **Phase 10** | Polish & Comprehensive Test Suite | Automated regression testing, stress testing, edge-case coverage | Planned |

---

## Directory Structure

```text
mini-unix-shell/
├── include/           # Header files and interface declarations
│   ├── command.hpp    # Command and pipeline data representations
│   ├── executor.hpp   # Process execution layer (fork, exec, waitpid)
│   ├── parser.hpp     # Whitespace and pipeline tokenizer interface
│   ├── shell.hpp      # REPL coordinator and prompt management
│   ├── builtin.hpp    # Built-in command interface
│   ├── job.hpp        # Job control data structures and JobManager
│   └── version.hpp    # Version and build metadata
├── src/               # Implementation source files
│   ├── executor.cpp   # POSIX fork/exec/waitpid and process groups
│   ├── main.cpp       # CLI entrypoint and options handling
│   ├── parser.cpp     # Command and pipeline tokenizer implementation
│   ├── shell.cpp      # REPL loop, signal dispositions, and terminal control
│   ├── builtin.cpp    # In-process built-in implementations
│   └── job.cpp        # Job table and process tracking implementation
├── tests/             # Automated test scripts and harnesses
│   └── run_tests.sh   # Phase regression and compliance test runner
├── docs/              # In-depth architectural and phase documentation
│   ├── phase-00-setup.md
│   ├── phase-01-process-execution.md
│   ├── phase-02-builtins.md
│   └── phase-08-job-control.md
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
mini-shell$ sleep 100 &
[1] 49123
mini-shell$ jobs
[1] running sleep 100 &
mini-shell$ fg 1
sleep 100 &
^Z
[1] stopped sleep 100 &
mini-shell$ bg 1
[1] sleep 100 &
mini-shell$ jobs
[1] running sleep 100 &
```

To exit the shell, press `Ctrl+D` (EOF) or type `exit`.

---

## Testing

The project maintains an automated test suite located in `tests/`. Each phase introduces test assertions validating functionality, error conditions, and regressions.

Run the test suite using:

```bash
make test
```

---

## Current Status & Limitations

- **Current Phase**: Phase 8 (Job Control).
- **Implemented**: Interactive REPL, whitespace parsing, `fork()`, `execvp()`, `waitpid()`, exit-status propagation, EOF handling, non-interactive input detection, in-process built-ins (`cd`, `pwd`, `exit`, `help`, `jobs`, `fg`, `bg`), `<`, `>`, and `>>` I/O redirection, pipelines via `pipe()`, background command/pipeline execution with `&`, signal handling (`SIGINT`, `SIGCHLD`, `SIGTSTP`, `SIGTTOU`, `SIGTTIN`), process groups via `setpgid()`, terminal control via `tcsetpgrp()`, and full job table management.
- **Next Step**: Phase 9 — Robustness, Error Handling, and Resource Management.

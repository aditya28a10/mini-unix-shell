# Phase 0 — Project Setup & Architecture Foundation

## 1. Goal

The objective of Phase 0 is to establish a disciplined, maintainable, and standards-compliant engineering foundation for the `mini-shell` project. This includes setting up the directory hierarchy, configuring a robust POSIX-compatible Makefile with dependency tracking, implementing a minimal smoke-testable entry point, establishing comprehensive documentation standards, and ensuring automated test verification without prematurely introducing Phase 1 process execution logic.

---

## 2. Features Implemented

- **Directory Hierarchy**: Clean separation of interface (`include/`), implementation (`src/`), automated tests (`tests/`), and architecture documentation (`docs/`).
- **POSIX Makefile**:
  - Incremental compilation with automated header dependency tracking (`-MMD -MP`).
  - Separation of compilation artifacts (`obj/`) from distribution binaries (`bin/`).
  - Strict compiler diagnostic flags (`-Wall -Wextra -Wpedantic -std=c++17`).
  - Standard development targets: `all`, `run`, `test`, `clean`, `help`.
- **Minimal Phase 0 Entrypoint**:
  - Program banner and status reporting confirming clean initialization.
  - Command-line option parsing for `-v` / `--version` and `-h` / `--help`.
  - Proper POSIX exit statuses (`0` for success, `1` for invalid flags).
- **Automated Phase 0 Test Suite**:
  - Bash test harness (`tests/run_tests.sh`) integrated into `make test`.
  - Validates compilation, binary permissions, banner output, flag handling, clean targets, and rebuild integrity.
- **Repository Hygiene**:
  - Comprehensive `.gitignore` preventing build artifacts, object files, editor temp files, and OS files from polluting version control.

---

## 3. Architecture

Phase 0 sets the baseline architectural structure for all subsequent phases:

```text
mini-unix-shell/
├── include/
│   └── version.hpp     # Compile-time constants and version metadata
├── src/
│   └── main.cpp        # CLI entry point, argument parsing, status dispatch
├── obj/                # Relocatable object files (*.o) and dependencies (*.d) [generated]
├── bin/                # Linked executable binaries [generated]
├── tests/
│   └── run_tests.sh    # Regression and compliance test harness
├── docs/
│   └── phase-00-setup.md # Technical phase documentation
├── LEARNING.md         # Systems programming and OS study companion
├── README.md           # Public repository overview
├── Makefile            # POSIX build definition
└── .gitignore          # Version control ignore definitions
```

### Build Pipeline Flow
1. Source files (`src/*.cpp`) are preprocessed, compiled with C++17 flags and include paths (`-Iinclude`), and assembled into object files (`obj/*.o`).
2. Dependency files (`obj/*.d`) are generated during compilation to track header modifications.
3. Object files are linked via the compiler driver (`g++` / `clang++`) into the final binary `bin/mini-shell`.

---

## 4. Important Source Files

| File | Purpose |
| :--- | :--- |
| `include/version.hpp` | Single source of truth for shell name (`mini-shell`), version (`0.0.1-phase0`), and description. |
| `src/main.cpp` | Phase 0 entrypoint handling CLI flags and reporting system initialization state. |
| `Makefile` | Declarative build system managing compilation rules, flags, and targets. |
| `tests/run_tests.sh` | Shell script verifying executable generation, exit codes, and cleanup routines. |

---

## 5. Data Structures Used

Phase 0 maintains minimal runtime state:
- `std::string_view` (C++17): Used for zero-allocation, non-owning string references when inspecting CLI arguments (`argv[1]`) and static configuration strings (`SHELL_NAME`, `SHELL_VERSION`).

---

## 6. POSIX / System Calls Used

In Phase 0, the program interacts with the OS via C++ runtime wrappers around underlying POSIX system calls:
- `write(2)` (via `std::cout` / `std::cerr`): Streaming character sequences to standard output and standard error.
- `_exit(2)` / `exit_group(2)` (invoked at program termination via `return` from `main`): Releasing process memory and returning status code to parent process.

---

## 7. Purpose of Each Important System Call

- **`write(fd, buf, count)`**: Transmits bytes from user-space memory buffers to kernel file description buffers (specifically `STDOUT_FILENO = 1` and `STDERR_FILENO = 2`).
- **`exit(status)`**: Flushes open stream buffers (e.g. `stdout`), calls registered `atexit` functions, destroys static objects, and instructs the kernel to terminate the calling thread/process with the given exit status.

---

## 8. Process Relationships

- **Parent Process**: The interactive shell (e.g., `bash`, `zsh`) or test script from which `./bin/mini-shell` is launched.
- **Child Process**: `mini-shell` executes as a direct child of the launching shell.
- **Termination & Status**: Upon returning `0` or `1` from `main()`, the kernel stores the lowest 8 bits of the status code in the process's PCB (Process Control Block). The parent process reads this via `waitpid(2)` or inspects `$?`.

---

## 9. File-Descriptor Relationships

Standard file descriptors inherited from parent process:
- `0` (`stdin` / `STDIN_FILENO`): Standard input (terminal keyboard).
- `1` (`stdout` / `STDOUT_FILENO`): Standard output (terminal display). Informational banners, version, and help strings are written here.
- `2` (`stderr` / `STDERR_FILENO`): Standard error (terminal display). Error notices (e.g., unrecognized options) are written here.

---

## 10. Design Decisions

1. **Why Makefile over CMake or Meson?**
   - In an undergraduate systems programming project, understanding Makefiles directly reinforces concepts of dependency graphs, file timestamps, compiler driver flags, and shell commands. Make is standard across POSIX environments without external tooling dependencies.
2. **Why `-MMD -MP` flags?**
   - Without automatic dependency tracking, modifying a header file like `include/version.hpp` would not trigger recompilation of `src/main.cpp` unless a full `make clean` was executed. `-MMD` generates Makefile rules for header dependencies automatically; `-MP` creates phony targets for each header to prevent build errors if a header is deleted.
3. **Why isolate `obj/` and `bin/`?**
   - Keeping generated binaries and object files separate from source files prevents clutter, simplifies `.gitignore`, and makes cleanup (`make clean`) trivial and deterministic.
4. **Why `std::string_view`?**
   - Avoids heap allocations (`std::string`) during argument checking. Idiomatic modern C++ that is both efficient and clean.

---

## 11. Error Handling

- **Invalid CLI Arguments**: If a user passes an unrecognized argument (e.g., `./bin/mini-shell --bogus`), the program writes a descriptive error message to `std::cerr` (file descriptor 2) and exits with status code `1` (`EXIT_FAILURE`), adhering to standard Unix utility behavior.

---

## 12. Test Cases

Automated in `tests/run_tests.sh`:
1. **Binary Existence**: Verifies `bin/mini-shell` was compiled and has executable permissions (`test -x`).
2. **Default Run**: Executes `./bin/mini-shell`, asserts exit code `0`, and checks that stdout contains the initialization banner.
3. **Version Flag**: Executes `./bin/mini-shell --version` and `-v`, asserts exit code `0`, and checks for exact version substring.
4. **Help Flag**: Executes `./bin/mini-shell --help` and `-h`, asserts exit code `0`, and checks for usage description.
5. **Invalid Flag**: Executes `./bin/mini-shell --invalid`, asserts non-zero exit code (`1`).
6. **Clean Target**: Executes `make clean` and asserts that `bin/` and `obj/` directories are completely removed.
7. **Rebuild Target**: Executes `make all` following `clean` and asserts that `bin/mini-shell` is recreated successfully.

---

## 13. Edge Cases

- **No Arguments**: Correctly identifies `argc == 1` and prints startup state instead of indexing out of bounds on `argv`.
- **Multiple Arguments**: Accurately inspects `argv[1]` without buffer overrun.

---

## 14. Known Limitations

- The interactive Read-Eval-Print Loop (REPL), command tokenizer, and process execution (`fork` / `execvp` / `waitpid`) are intentionally not implemented in Phase 0. They will be designed and implemented in Phase 1.

---

## 15. Operating Systems & Computer Architecture Concepts Demonstrated

- **Build Systems as DAGs**: How build tools compute topological sorts on dependency graphs.
- **Virtual Memory Image Preparation**: How the compiler and linker emit ELF/Mach-O sections (text, data, rodata) ready to be loaded by the OS kernel `execve(2)` loader.
- **Process Exit Semantics**: How integer status codes are propagated from user-space execution back to kernel process management tables.

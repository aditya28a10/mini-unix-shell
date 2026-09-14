# Phase 1 — Basic Command Execution

## 1. Goal

The objective of Phase 1 is to transform `mini-shell` into a functioning Unix command-line shell capable of reading interactive commands, parsing them into discrete argument vectors, and executing external programs via fundamental POSIX process creation and lifecycle management primitives (`fork`, `execvp`, `waitpid`).

---

## 2. Features Implemented

- **Interactive Shell Prompt**:
  - Displays `mini-shell$ ` when standard input is attached to an interactive terminal (`isatty(STDIN_FILENO)`).
  - Flushes prompt buffer immediately to ensure responsiveness.
  - Automatically suppresses prompt when running non-interactively (e.g. redirected or piped input), enabling automated test scripting.
- **Input Reading & EOF Handling**:
  - Reads line-by-line input using `std::getline`.
  - Cleanly exits on End-Of-File (EOF / `Ctrl+D`), outputting a newline in interactive mode to preserve terminal cleanliness.
- **Whitespace Command Parsing**:
  - Tokenizes input lines strictly based on whitespace delimiters (spaces, tabs, newlines) using `std::istringstream`.
  - Ignores leading, trailing, and repeated whitespace.
  - Skips execution on blank or whitespace-only lines without error.
- **Process Creation & Execution (`fork`, `execvp`, `waitpid`)**:
  - Converts parsed tokens into null-terminated `char* const argv[]` arrays required by POSIX `exec`.
  - Spawns a child process using `fork()`.
  - Executes the requested program in the child using `execvp()`, leveraging the system `PATH` environment variable.
  - Synchronously waits for child termination in the parent using `waitpid()`.
- **Exit Status Collection & Error Handling**:
  - Reaps child termination status using POSIX macros (`WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`, `WTERMSIG`).
  - Distinguishes standard POSIX failure conditions (`ENOENT` -> exit 127, `EACCES` -> exit 126).
  - Employs `_exit()` rather than standard `exit()` in the child upon `execvp()` failure to avoid flushing parent I/O buffers or executing parent `atexit` destructors.

---

## 3. Architecture

Phase 1 establishes a modular 4-component architecture:

```text
               +----------------------------------+
               |         minishell::Shell         |  <--- REPL Loop
               +-----------------+----------------+
                                 | (reads input line)
                                 v
               +----------------------------------+
               |        minishell::Parser         |  <--- Whitespace Tokenizer
               +-----------------+----------------+
                                 | (produces Command)
                                 v
               +----------------------------------+
               |       minishell::Executor        |  <--- POSIX Process Layer
               +--------+----------------+--------+
                        |                |
                fork()  |                | waitpid()
                        v                v
                 [Child Process]   [Parent Shell]
                        |                |
             execvp()   |                | blocks until child exits
                        v                v
                 [Program Image]   [Reaps Exit Status]
```

---

## 4. Important Source Files

| File | Role |
| :--- | :--- |
| `include/command.hpp` | Data structure representing a parsed command (`args` vector, helper methods). |
| `include/parser.hpp` / `src/parser.cpp` | Whitespace-based tokenizer extracting command and arguments. |
| `include/executor.hpp` / `src/executor.cpp` | Encapsulates POSIX process lifecycle: `fork()`, `execvp()`, `waitpid()`, error handling. |
| `include/shell.hpp` / `src/shell.cpp` | Read-Eval-Print Loop (REPL), prompt rendering, TTY detection, EOF handling. |
| `src/main.cpp` | Entrypoint maintaining CLI options (`--version`, `--help`) and launching `Shell::run()`. |
| `tests/run_tests.sh` | Automated test suite verifying regression and Phase 1 functionality. |

---

## 5. Data Structures Used

- **`minishell::Command`**:
  ```cpp
  struct Command {
      std::vector<std::string> args;
      bool empty() const;
      const std::string& name() const;
  };
  ```
  Encapsulates the parsed argument vector where `args[0]` is the executable name/path and `args[1..n]` are arguments.
- **`std::vector<char*>` (`c_argv`)**:
  Temporary contiguous pointer vector created during execution. C++ `std::string` objects are converted into C-style pointers (`arg.c_str()`) and terminated with `nullptr` to conform to POSIX `execvp(const char* file, char* const argv[])` requirements.

---

## 6. POSIX / System Calls Used

| System Call | Header | Signature |
| :--- | :--- | :--- |
| `fork()` | `<unistd.h>` | `pid_t fork(void);` |
| `execvp()` | `<unistd.h>` | `int execvp(const char *file, char *const argv[]);` |
| `waitpid()` | `<sys/wait.h>` | `pid_t waitpid(pid_t pid, int *status, int options);` |
| `_exit()` | `<unistd.h>` | `void _exit(int status);` |
| `isatty()` | `<unistd.h>` | `int isatty(int fd);` |

---

## 7. Purpose of Each Important System Call

### `fork()`
Creates a new process by duplicating the calling process.
- The new process (the **child**) is an exact duplicate of the calling process (the **parent**), except for a few distinct properties (such as unique PID, PPID, and separate memory page tables).
- **Return Value**:
  - `0` in the child process.
  - The positive PID of the child process in the parent process.
  - `-1` if process creation failed (e.g. system PID limit or memory exhaustion).

### `execvp()`
Replaces the current process image with a new process image loaded from an executable file.
- The `p` variant searches the directories listed in the `PATH` environment variable if the specified file name does not contain a slash (`/`).
- The `v` variant accepts command-line arguments as an array of pointers to null-terminated strings (`char* const argv[]`), where `argv[argc] == NULL`.
- If successful, `execvp()` **never returns** because the calling program's text, data, bss, and stack are entirely overwritten by the new program.
- If `execvp()` returns, execution failed (returns `-1` and sets `errno`).

### `waitpid()`
Suspends the calling process until a child process specified by `pid` has changed state (terminated, stopped, or resumed).
- `waitpid(pid, &status, 0)` blocks synchronously until the specific child process terminates.
- Populates the integer status variable with exit information reaped from the kernel.

### `_exit()`
Immediately terminates the calling process without performing user-space cleanup:
- Does **not** call `atexit()` handlers.
- Does **not** flush standard C/C++ I/O stream buffers (`std::cout`, `stdout`).
- Directly invokes the kernel's termination routine (`sys_exit`). This is critical when `execvp` fails in a forked child to prevent corrupting shared parent stream buffers.

### `isatty()`
Queries whether a given open file descriptor refers to an interactive terminal device. Used to determine if the prompt should be displayed.

---

## 8. Process Relationships

- **Parent Process (`mini-shell`)**:
  The parent process runs the main REPL loop. It parses user input, calls `fork()`, and waits for the child process to complete using `waitpid()`. The parent remains alive throughout the interactive session.
- **Child Process**:
  Spawned by `fork()`. It temporarily shares the open file descriptors of the parent shell. It immediately calls `execvp()` to transform itself into the target program (e.g. `ls`, `grep`, `pwd`). Once finished, the child terminates, leaving its exit status in the kernel until reaped by the parent.

```text
[Parent: mini-shell (PID: 1000)]
       |
       | fork()
       +------------------------------------+
       |                                    |
       v                                    v
[Parent: mini-shell]              [Child (PID: 1001)]
       |                                    |
       | waitpid(1001, &status, 0)          | execvp("ls", ["ls", NULL])
       | (blocks...)                        v
       |                          [Overwritten with /bin/ls]
       |                                    |
       |                                    | executes and terminates (exit 0)
       | <----------------------------------+
       | (reaps exit status 0)
       v
[Prints prompt mini-shell$ ]
```

---

## 9. File-Descriptor Relationships

In Phase 1, the child process inherits all standard file descriptors directly from the parent shell:
- `STDIN_FILENO (0)`: Connected to the terminal keyboard (or piped input).
- `STDOUT_FILENO (1)`: Connected to the terminal display. Output from the executed command streams directly to the user.
- `STDERR_FILENO (2)`: Connected to the terminal display. Error messages stream directly to the user.

No descriptor redirection or manipulation is performed in Phase 1 (reserved for Phase 3).

---

## 10. Design Decisions

1. **Why `execvp` instead of `execlp` or `execve`?**
   - The shell receives arguments dynamically at runtime as an array/vector of strings. Vector-based `execv*` functions accept variable-length argument arrays, whereas `execl*` functions require fixed argument lists at compile-time.
   - `execvp` handles `PATH` resolution automatically, allowing commands like `ls` to execute without requiring the user to type `/bin/ls`.
2. **Why `_exit()` instead of `exit()` on exec failure?**
   - In modern POSIX systems, calling `exit()` in a child process flushes C/C++ I/O stream buffers. If the child inherited unflushed buffers from the parent process, `exit()` would flush them a second time, resulting in duplicate output. Calling `_exit()` bypasses user-space buffer flushing.
3. **Why check `isatty(STDIN_FILENO)`?**
   - Automated testing and Unix pipelining rely on passing commands via stdin (`cat script.sh | ./bin/mini-shell`). Suppressing the prompt when `stdin` is not a terminal prevents polluting stdout with prompt strings.
4. **Why `std::istringstream` for Phase 1 tokenization?**
   - Scoped strictly to Phase 1 requirements: standard whitespace separation without over-engineering quote parsing or escape rules prematurely.

---

## 11. Error Handling

1. **Fork Failure (`pid < 0`)**:
   - Condition: System PID table full (`EAGAIN`) or kernel out of memory (`ENOMEM`).
   - Action: `perror("mini-shell: fork")`, returns `-1` to caller, shell stays responsive.
2. **Exec Failure**:
   - Condition: Command does not exist (`ENOENT`), target file lacks execute permissions (`EACCES`), or invalid binary format.
   - Action: Prints error via `perror()`. Terminates child with code `127` (`ENOENT`), `126` (`EACCES`), or `1` using `_exit()`.
3. **Waitpid Failure (`wpid < 0`)**:
   - Condition: Child was reaped or interrupted.
   - Action: `perror("mini-shell: waitpid")`.
4. **Status Evaluation**:
   - Validates if child exited normally via `WIFEXITED(status)` before reading `WEXITSTATUS(status)`.
   - Handles signal termination via `WIFSIGNALED(status)` and returns `128 + WTERMSIG(status)`.

---

## 12. Test Cases

All automated in `tests/run_tests.sh`:
- `test_basic_echo`: Verifies execution of single external command (`echo hello`).
- `test_multi_args`: Verifies multiple whitespace-separated arguments (`echo foo bar baz`).
- `test_external_pwd`: Verifies system utility execution (`pwd`).
- `test_whitespace_and_blank_lines`: Verifies leading, trailing, and multi-line whitespace handling.
- `test_sequential_commands`: Verifies executing sequential commands in stream order.
- `test_eof_termination`: Verifies clean exit on EOF (`Ctrl+D`).
- `test_nonexistent_command_error`: Verifies error reporting on invalid commands.
- `test_command_failure_exit_status`: Verifies exit code propagation on command failure.

---

## 13. Edge Cases

- **Blank lines and repeated whitespace**: Handled cleanly by `iss >> token`; if empty, no fork occurs.
- **Command does not exist**: Child catches `ENOENT` from `execvp`, prints descriptive error, and terminates without crashing parent shell.
- **Empty input / EOF**: `std::getline` returns false, shell breaks from loop and terminates cleanly.

---

## 14. Known Limitations

- **Built-in commands (`cd`, `pwd`, `exit`)**: In Phase 1, commands like `cd` cannot change the shell's working directory because external programs run inside child processes. Built-ins will be implemented in Phase 2.
- **Quoted arguments**: Strings with spaces inside quotes (e.g. `echo "hello world"`) are split into separate tokens.
- **Redirection and Pipes**: Operators like `<`, `>`, and `|` are treated as literal argument tokens.

---

## 15. Operating Systems Concepts Demonstrated

### Process vs. Program
- A **program** is passive executable code residing as an ELF or Mach-O binary file on disk.
- A **process** is an active, executing instance of a program residing in virtual memory with allocated kernel resources (PID, page tables, registers, open file descriptor table, and stack/heap memory).

### Parent vs. Child Process
- The shell is the parent process.
- When executing external programs, the shell forks a child process. The child process possesses an independent virtual address space, preventing bugs or crashes in external commands from corrupting or terminating the parent shell.

### The Fork-Exec Pattern
- In Unix, process creation and program execution are deliberately separated into two orthogonal primitives:
  1. `fork()` creates a new process (allocates PCB, duplicates address space).
  2. `exec()` replaces the process image with a new program.
- This design allows the shell to configure the execution environment (file descriptor redirection, signal dispositions, process groups) in the child process *after* `fork()` but *before* `exec()`.

### Zombie Processes and Reaping
- When a child process terminates, it enters the `ZOMBIE` state. Its memory is freed, but its PID and exit status remain in the kernel process table until the parent process calls `waitpid()` to read the status.
- Calling `waitpid()` synchronously reaps the child, preventing zombie process leaks.

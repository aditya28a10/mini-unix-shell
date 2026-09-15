# Phase 2 — Built-in Commands

## 1. Goal
Implement shell commands that must execute in the shell process itself while preserving the Phase 1 fork/exec/wait path for external commands.

## 2. Features
- `cd [dir]`
- `pwd`
- `exit [status]`
- `help`
- Built-in dispatch before external command execution
- Error handling and exit-status propagation

## 3. Architecture
The REPL parses a command, checks `is_builtin()`, and either executes the built-in in-process or calls the existing external executor.

## 4. Why Built-ins Need In-process Execution
`cd` changes the current working directory of the shell process. A child created by `fork()` inherits the directory, but changing that child with `chdir()` would not change the parent shell. `exit` similarly has to terminate the shell process rather than a temporary child.

## 5. `cd`
With no argument, `cd` uses `HOME`. With one argument it calls `chdir()` on the supplied path. More than one argument is rejected. `chdir()` failures are reported with `perror()`.

## 6. `pwd`
`pwd` calls `getcwd()` and prints the returned path. The implementation treats the actual current working directory as authoritative rather than relying on `$PWD`.

## 7. `exit`
`exit` requests termination of the shell loop. With no argument it returns the most recent command status; with a numeric argument it returns that value. Too many arguments are rejected. A non-numeric argument requests termination with status 2.

## 8. `help`
`help` prints the supported built-ins and explains the difference between in-process built-ins and external commands.

## 9. External Commands
Commands that are not built-ins continue to use Phase 1: `fork()`, child `execvp()`, parent `waitpid()`, and exit-status inspection.

## 10. Data Flow
```text
input -> parser -> Command -> built-in dispatch
                              |             |
                           built-in      external
                              |             |
                         shell process   fork/exec/wait
```

## 11. Important APIs
- `chdir(2)` — change the calling process's current working directory.
- `getcwd(3)` — obtain the current working directory.
- `getenv(3)` — read `HOME`.
- `fork(2)`, `execvp(3)`, `waitpid(2)` — preserved from Phase 1 for external commands.

## 12. Error Handling
System-call failures are reported with `perror()`. Invalid built-in arguments produce explicit diagnostics and non-zero status codes. `exit` validates its optional numeric argument.

## 13. Tests
The test suite retains all Phase 0 and Phase 1 tests and adds coverage for `pwd`, directory changes, `HOME`, `cd` errors, `help`, `exit` statuses, exit argument errors, and persistence of `cd` state.

## 14. Edge Cases and Limitations
Phase 2 does not implement `cd -`, redirection, pipes, background execution, signals, job control, history, or shell quoting/escaping. `cd` with no argument requires a non-empty `HOME` environment variable.

## 15. OS / Interview Takeaways
The central lesson is that not every shell command should be implemented as a child process. Process-local kernel state, especially the current working directory, determines where an operation must execute. This phase also reinforces the distinction between a shell's user-space code and kernel-managed process state.

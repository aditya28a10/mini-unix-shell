# Phase 3 — I/O Redirection

## Goal

Add basic POSIX I/O redirection to the shell so commands can read input from files and write output to files.

## Implemented Features

- `<` redirects standard input from a file.
- `>` redirects standard output to a file, replacing existing contents.
- `>>` redirects standard output to a file while preserving existing contents.
- Redirection is applied in the child process before `execvp()`.
- Redirection failures are reported without terminating the shell itself.

## Architecture

For an external command:

1. The parser identifies redirection operators and their filenames.
2. The shell creates the child process using `fork()`.
3. The child opens the required file using `open()`.
4. The child connects the opened file to standard input or output using `dup2()`.
5. The child closes the original file descriptor using `close()`.
6. The child executes the requested program using `execvp()`.
7. The parent waits for the child using `waitpid()`.

Conceptually:

```text
Command
   |
   v
Parser
   |
   +---- arguments
   |
   +---- redirections
             |
             v
           fork()
             |
        +----+----+
        |         |
      Child     Parent
        |         |
      open()   waitpid()
        |
      dup2()
        |
      close()
        |
      execvp()
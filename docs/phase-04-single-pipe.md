# Phase 4 — Single Pipe

## Overview

Phase 4 adds single-pipeline support to the mini Unix shell.

The shell can connect the standard output of one command to the standard input of another command using the POSIX `pipe()` system call.

Example:

echo hello | wc -c

The data flow is:

Command 1
   |
   | stdout
   v
pipe()
   |
   | stdin
   v
Command 2

## Core Concepts

### pipe()

The POSIX `pipe()` system call creates a unidirectional communication channel with two file descriptors:

pipefd[0] -> read end
pipefd[1] -> write end

Data written to the write end can be read from the read end.

The shell uses this mechanism to connect two processes.

## Process Creation

A pipeline requires two child processes.

Conceptually:

Shell
 |
 +-- fork() --> Child 1
 |
 +-- fork() --> Child 2

The first child executes the command on the left side of `|`.

The second child executes the command on the right side.

## dup2() and Standard Streams

For:

command1 | command2

the shell redirects:

command1 stdout -> pipe write end
command2 stdin  <- pipe read end

This is accomplished using `dup2()`.

For the first child:

dup2(pipefd[1], STDOUT_FILENO);

For the second child:

dup2(pipefd[0], STDIN_FILENO);

After the descriptors have been duplicated, the children can execute normally with `execvp()`.

## File Descriptor Management

Correctly closing unused pipe descriptors is essential.

The first child only needs the write end.

The second child only needs the read end.

The shell itself must also close both ends after creating the children.

Conceptually:

Child 1:
    use write end
    close read end

Child 2:
    use read end
    close write end

Parent:
    close read end
    close write end

If unused pipe descriptors remain open, the reader may not receive EOF when expected. This can cause commands such as `cat` to wait indefinitely.

## Execution Flow

For a command such as:

ls | wc -l

the shell performs approximately these steps:

1. Parse the pipeline.
2. Create a pipe using `pipe()`.
3. Fork the first child.
4. Connect the first child's stdout to the pipe's write end.
5. Fork the second child.
6. Connect the second child's stdin to the pipe's read end.
7. Close unused pipe descriptors.
8. Execute both commands using `execvp()`.
9. Wait for the child processes using `waitpid()`.

The shell therefore coordinates two independent processes while the operating system provides the communication channel.

## Why execvp() Is Used

`fork()` creates the child process, but it does not change which program the child executes.

The child therefore uses `execvp()` to replace its process image with the requested command.

Conceptually:

fork()
  |
  v
child process
  |
  v
dup2()
  |
  v
execvp()

If `execvp()` fails, the child reports the error and exits with an appropriate failure status.

## Error Handling

The implementation must handle failures from system calls such as:

pipe()
fork()
dup2()
execvp()
waitpid()

A failure must not leave the shell in a broken state.

In particular, pipe descriptors must be closed appropriately when pipeline creation or execution fails.

## Example

Input:

echo hello | wc -c

Conceptually:

+---------+       pipe       +---------+
|  echo   |  ------------->  |   wc    |
| stdout  |                  |  stdin  |
+---------+                  +---------+

`echo` writes into the pipe, and `wc` reads from it.

## Relationship to Phase 5

Phase 4 implements the fundamental single-pipe mechanism:

command1 | command2

Phase 5 extends this mechanism to multiple pipeline stages:

command1 | command2 | command3 | ... | commandN

The same fundamental concepts remain:

- `pipe()`
- `fork()`
- `dup2()`
- `close()`
- `execvp()`
- `waitpid()`

Phase 5 generalizes the file-descriptor management to an arbitrary number of pipeline stages.

## OS Concepts Demonstrated

Phase 4 demonstrates several important operating-system concepts:

- Inter-process communication (IPC)
- File descriptors
- Anonymous pipes
- Process creation
- Process replacement
- Standard input/output
- File descriptor duplication
- Parent/child process coordination
- Process synchronization

## Interview Questions

### Why is pipe() needed?

A pipe provides a kernel-managed communication channel between processes. It allows the output of one process to become the input of another.

### Why use dup2()?

`dup2()` makes the pipe's file descriptor available through the standard stream expected by the command.

### Why must unused pipe descriptors be closed?

Keeping unused descriptors open can prevent EOF from reaching the reading process and can cause commands to block unexpectedly.

### Why are two child processes required?

Each command in the pipeline executes as a separate process. The shell creates a process for each command and connects them through the pipe.

### What happens if execvp() fails?

The child cannot execute the requested program, so it reports the error and exits. The parent continues managing the remaining pipeline processes and returns to the shell normally.

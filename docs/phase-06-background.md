# Phase 6 — Background Processes

## What Changed

Phase 5 supported foreground pipelines where the shell waits for all child processes to finish.

Phase 6 adds background execution using `&`.

Example:

sleep 5 &

The shell starts the process and immediately returns to the prompt instead of waiting for it.

---

## 1. Background Flag

The `Command` structure now contains:

bool background = false;

The parser recognizes:

&

as a background marker instead of passing it to `execvp()`.

For:

sleep 5 &

the command arguments are:

sleep
5

and:

background = true

---

## 2. Foreground vs Background

Foreground command:

sleep 5

The parent waits:

waitpid(pid, &status, 0);

The shell does not continue until the child finishes.

Background command:

sleep 5 &

The parent does not perform a blocking wait.

Instead, it reports the child PID and immediately continues the shell loop.

---

## 3. Reaping Background Processes

Background processes must eventually be reaped.

Otherwise, a terminated child can remain as a zombie process.

The shell periodically calls:

waitpid(-1, &status, WNOHANG);

`WNOHANG` means:

- return immediately if no child has finished
- return the PID if a child has finished
- do not block the shell

The shell performs this check at the beginning of each REPL iteration.

---

## 4. Background Pipelines

Phase 6 also supports:

command1 | command2 &

For example:

sleep 5 | cat &

The entire pipeline runs in the background.

The shell closes its pipe descriptors and returns immediately instead of waiting for the pipeline.

The pipeline's child processes are later reaped by the background cleanup logic.

---

## 5. Why `&` Is Not Passed to `execvp()`

The shell interprets `&`.

It is shell syntax, not an argument for the external program.

For:

echo hello &

the program receives:

argv[0] = "echo"
argv[1] = "hello"

It does not receive:

argv[2] = "&"

---

## 6. Current Design

Background processes are currently tracked through the operating system's child-process relationship rather than a persistent shell job table.

The shell reports the PID when starting a background command.

When finished children are detected, the shell reports that they have been reaped.

---

## Current Limitations

Phase 6 does not yet implement:

- `jobs`
- `fg`
- `bg`
- persistent job IDs
- process groups
- terminal foreground control
- `SIGCHLD` handling
- `SIGINT` / `SIGTSTP` handling
- full shell grammar
- quoting and escaping
- pipeline + redirection semantics
- background execution for built-in commands

These are addressed by later phases where appropriate.

---

## Important OS Concepts

Phase 6 connects:

- Background processes
- Parent/child process relationships
- `fork()`
- `waitpid()`
- `WNOHANG`
- Process lifecycle
- Zombie processes
- Anonymous pipes
- `dup2()`
- `execvp()`

---

## Key Takeaway

A background process allows the shell to continue without waiting for the child to finish.

The important distinction is:

Foreground:
fork → exec → blocking wait

Background:
fork → exec → continue shell → later reap child

The shell must still reap terminated background children to prevent zombies.

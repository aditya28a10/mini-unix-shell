# Phase 7 — Signal Handling

## What Changed

Phase 7 adds basic POSIX signal handling to the shell.

The shell now handles:

- `SIGINT`
- `SIGCHLD`

The implementation uses `sigaction()` and `sig_atomic_t` flags.

The shell can survive `Ctrl+C`, foreground commands can terminate normally from `SIGINT`, and background children can be detected and reaped through `SIGCHLD`.

---

## 1. SIGINT Handling

`Ctrl+C` generates:

SIGINT

The shell installs a `SIGINT` handler using:

sigaction()

The handler does not perform complex work.

Instead, it sets:

volatile sig_atomic_t g_sigint_received = 1;

The normal shell loop then handles the event.

This avoids performing unsafe operations such as using `std::cout` directly from a signal handler.

---

## 2. Shell Survives Ctrl+C

When the shell is waiting for user input:

Ctrl+C

should not terminate the shell.

The signal handler records that `SIGINT` was received.

The shell then clears the interrupted input state and continues the REPL.

Example:

mini-shell$ ^C
mini-shell$

The shell remains available for another command.

---

## 3. Child SIGINT Behavior

After `fork()`, the child initially inherits the shell's signal dispositions.

Before executing an external command, the child restores the default behavior for `SIGINT`:

sa.sa_handler = SIG_DFL;

This allows external programs to respond to `Ctrl+C` normally.

The shell itself keeps its custom `SIGINT` handling.

The intended relationship is:

shell:
SIGINT → shell handler

child:
SIGINT → default behavior

---

## 4. Interrupted waitpid()

Signals can interrupt blocking system calls.

For example:

waitpid(pid, &status, 0);

may return:

-1

with:

errno == EINTR

`EINTR` means that the system call was interrupted by a signal.

The shell now retries `waitpid()` when this happens.

Example:

do {
    wpid = waitpid(pid, &status, 0);
} while (wpid == -1 && errno == EINTR);

This behavior is implemented for:

- foreground commands
- foreground pipelines

---

## 5. SIGCHLD

When a child process terminates, the operating system can send:

SIGCHLD

The shell installs a `SIGCHLD` handler.

The handler only records the event:

volatile sig_atomic_t g_sigchld_received = 1;

The handler does not call `waitpid()` or perform output.

The normal shell code performs the actual child reaping.

---

## 6. Background Child Reaping

Phase 6 already provided:

reap_background_processes()

This function uses:

waitpid(-1, &status, WNOHANG);

Phase 7 connects this mechanism with `SIGCHLD`.

The flow is:

child terminates
        ↓
SIGCHLD
        ↓
signal handler
        ↓
g_sigchld_received = 1
        ↓
normal shell code
        ↓
reap_background_processes()
        ↓
waitpid(..., WNOHANG)

This allows the shell to remain responsive while preventing terminated background children from remaining as zombies.

---

## 7. Signals and getline()

The shell's input loop uses:

std::getline(std::cin, line);

A signal can interrupt the input operation.

The shell checks the signal flags when input fails.

For `SIGINT`, the shell:

- clears the stream state
- resets the signal flag
- continues the shell loop

For `SIGCHLD`, the shell:

- clears the stream state
- resets the signal flag
- reaps terminated children
- continues the shell loop

Actual EOF is still handled separately.

This distinction is important because an interrupted input operation should not be treated as `Ctrl+D`.

---

## 8. Signal Handler Restrictions

Signal handlers execute asynchronously.

They should perform only simple, signal-safe operations.

This implementation therefore uses the handlers only to set:

volatile sig_atomic_t

flags.

Operations such as:

std::cout

and:

waitpid()

are performed by normal shell code instead of directly inside the handlers.

---

## 9. Foreground Command Example

For:

sleep 100

the shell waits for the child.

If `Ctrl+C` is pressed:

Ctrl+C
   ↓
SIGINT
   ↓
foreground child receives default SIGINT behavior
   ↓
child terminates
   ↓
shell's wait is handled correctly
   ↓
shell returns to the prompt

The shell itself remains alive.

---

## 10. Background Command Example

For:

sleep 2 &

the shell immediately returns to the prompt.

When the child terminates:

sleep 2 &
    ↓
child terminates
    ↓
SIGCHLD
    ↓
shell records the signal
    ↓
shell reaps the child
    ↓
background process is reported as finished

---

## 11. Pipeline Signal Handling

Phase 7 also handles interrupted waits for foreground pipelines.

For:

echo hello | cat

the shell waits for multiple child processes.

If a signal interrupts one of the waits, the shell retries `waitpid()` when:

errno == EINTR

This prevents a signal from being incorrectly treated as a permanent wait failure.

Background pipelines continue to be reaped through the existing background-child cleanup mechanism.

---

## Current Design

Phase 7 uses signal flags rather than performing complex work directly inside signal handlers.

The important separation is:

Signal handler:
detect event → set flag

Normal shell code:
inspect flag → perform required work

This keeps signal handling simple and makes the behavior easier to reason about.

---

## Current Limitations

Phase 7 does not yet implement:

- process groups
- terminal foreground process control
- `setpgid()`
- `tcsetpgrp()`
- `fg`
- `bg`
- persistent job IDs
- `jobs`
- `SIGTSTP` job suspension
- full job-control semantics

These belong to the job-control phase.

The shell also does not yet maintain a persistent job table for background pipelines.

---

## Important OS Concepts

Phase 7 connects:

- POSIX signals
- `SIGINT`
- `SIGCHLD`
- `sigaction()`
- `sig_atomic_t`
- signal handlers
- interrupted system calls
- `EINTR`
- `SIG_DFL`
- `waitpid()`
- `WNOHANG`
- child-process lifecycle
- zombie prevention

---

## Key Takeaway

Signals are asynchronous events delivered by the operating system.

A robust shell should separate:

signal detection

from:

signal handling work.

In this phase, signal handlers only record events using `sig_atomic_t` flags.

The normal shell loop then performs operations such as:

- clearing interrupted input
- waiting for children
- reaping background children

This provides the foundation required for proper process-group and terminal job control in the next phase.

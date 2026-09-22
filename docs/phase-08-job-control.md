# Phase 8 — Job Control

## 1. Goal

The objective of Phase 8 is to implement authentic, POSIX-compliant terminal Job Control in `mini-shell`. This allows the shell to manage multiple foreground and background execution tasks (both single commands and multi-process pipelines), handle process groups via `setpgid()`, control terminal access via `tcsetpgrp()`, track process states (`Running`, `Stopped`, `Done`), respond to suspension signals (`SIGTSTP` / Ctrl+Z), and provide built-in management utilities (`jobs`, `fg`, `bg`).

---

## 2. Features Implemented

- **Job Representation & Management**:
  - Encapsulation of single commands and multi-command pipelines into logical shell `Job` objects with unique, sequential Job IDs.
  - Tracking of child process states (`completed`, `stopped`, exit status) within each job.
- **Process Group Isolation**:
  - Explicit creation of distinct process groups for each command or pipeline using `setpgid()`.
  - Process group leader assignment to prevent signal bleeding and ensure proper group-wide signal delivery.
  - Symmetrical `setpgid()` invocation in both parent and child processes to eliminate race conditions.
- **Terminal Control & Ownership**:
  - Active management of controlling terminal foreground ownership via `tcsetpgrp()`.
  - Handoff of terminal access to foreground process groups and safe restoration to the shell upon process termination or suspension.
  - Defensive protection against `SIGTTOU` and `SIGTTIN` signals during terminal ownership handoffs.
- **Interactive Process Suspension (Ctrl+Z / `SIGTSTP`)**:
  - Detection of child process stop events via `waitpid(..., WUNTRACED)`.
  - Automatic addition/transition of suspended processes to `Stopped` state in the job table.
  - Notification output: `[<id>] stopped <command>`.
- **Job Control Built-ins**:
  - `jobs`: Lists active background and stopped jobs.
  - `fg [job_id]`: Transfers terminal ownership to the job, sends `SIGCONT` if stopped, and synchronously waits in foreground.
  - `bg [job_id]`: Resumes stopped jobs in background by sending `SIGCONT` to the process group.
- **Asynchronous Status Updates & Reaping**:
  - Integration with `SIGCHLD` and non-blocking `waitpid(-1, ..., WNOHANG | WUNTRACED | WCONTINUED)` to report completed background tasks before shell prompts.

---

## 3. Architecture

```text
                           +----------------------------------+
                           |         minishell::Shell         |  <--- REPL & Terminal Control
                           +-----------------+----------------+
                                             |
                   +-------------------------+-------------------------+
                   |                                                   |
                   v                                                   v
    +------------------------------+                    +------------------------------+
    |    minishell::JobManager     |                    |     minishell::Executor      |
    +------------------------------+                    +------------------------------+
    | - Job Table (Running/Stopped)|                    | - setpgid() (parent & child) |
    | - Job ID allocation          |                    | - tcsetpgrp() (terminal handoff)
    | - Asynchronous reaping       |                    | - waitpid(..., WUNTRACED)    |
    | - Status notifications       |                    | - kill(-pgid, SIGCONT)       |
    +------------------------------+                    +------------------------------+
                   |                                                   |
                   +-------------------------+-------------------------+
                                             |
                                             v
                             +-------------------------------+
                             |    Process Group (PGID)       |
                             |  [Child 1] | [Child 2] | ...  |
                             +-------------------------------+
```

---

## 4. Important Source Files

| File | Role |
| :--- | :--- |
| `include/job.hpp` / `src/job.cpp` | Defines `Job`, `ProcessInfo`, `JobState`, and `JobManager` singleton tracking active jobs and state transitions. |
| `include/executor.hpp` / `src/executor.cpp` | Implements process group configuration (`setpgid`), terminal control (`tcsetpgrp`), signal reset, and foreground job waiting (`wait_for_job`). |
| `include/builtin.hpp` / `src/builtin.cpp` | Implements built-in command handlers: `jobs`, `fg`, `bg`, `cd`, `pwd`, `exit`, `help`. |
| `src/shell.cpp` | REPL loop, signal ignoring for `SIGTSTP`/`SIGTTOU`/`SIGTTIN`, terminal restoration, and job notifications. |

---

## 5. Data Structures Used

- **`minishell::ProcessInfo`**:
  Tracks individual PID state within a job: `pid_t pid`, `bool completed`, `bool stopped`, `int status`.
- **`minishell::Job`**:
  Represents a logical execution unit (single command or full pipeline):
  - `int id`: Sequential job ID.
  - `pid_t pgid`: Shared process group ID.
  - `std::vector<ProcessInfo> processes`: All processes in the pipeline.
  - `std::string command`: Formatted command line string.
  - `JobState state`: Current state (`Running`, `Stopped`, `Done`, `Terminated`).
  - `bool background`: True if started with `&`.
  - `bool notified`: True if user was informed of termination/stop.
- **`minishell::JobManager`**:
  Maintains the global job table (`std::vector<Job>`), next job ID generator, and synchronization helpers.

---

## 6. POSIX / System Calls Used

| System Call | Header | Signature | Purpose |
| :--- | :--- | :--- | :--- |
| `setpgid()` | `<unistd.h>` | `int setpgid(pid_t pid, pid_t pgid);` | Assigns process group membership. |
| `tcsetpgrp()` | `<unistd.h>` | `int tcsetpgrp(int fd, pid_t pgrp);` | Sets foreground process group on controlling terminal. |
| `kill()` | `<signal.h>` | `int kill(pid_t pid, int sig);` | Sends signals (e.g. `SIGCONT` to `-pgid`). |
| `waitpid()` | `<sys/wait.h>` | `pid_t waitpid(pid_t pid, int *status, int options);` | Reaps and monitors processes with `WUNTRACED`, `WNOHANG`, `WCONTINUED`. |
| `sigaction()` | `<signal.h>` | `int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);` | Configures signal handlers and signal ignoring (`SIG_IGN`). |

---

## 7. Purpose of Each Important System Call

### `setpgid(pid, pgid)`
Sets the process group ID of process `pid` to `pgid`. If `pid == 0`, the calling process's PID is used. If `pgid == 0`, the PID of the process becomes its PGID, making it the process group leader. Calling `setpgid()` in both parent and child eliminates race conditions before `execvp()`.

### `tcsetpgrp(fd, pgrp)`
Transfers foreground terminal ownership of file descriptor `fd` (which must refer to the controlling terminal) to process group `pgrp`. Foreground processes receive keyboard input and terminal-generated signals (`SIGINT`, `SIGTSTP`).

### `kill(-pgid, SIGCONT)`
Passing a negative PID value (`-pgid`) directs the kernel to deliver the signal (`SIGCONT`) to every process in the specified process group simultaneously, resuming stopped pipelines in unison.

### `waitpid(..., WUNTRACED | WNOHANG | WCONTINUED)`
- `WUNTRACED`: Reports status when a child process stops (e.g. via `SIGTSTP`).
- `WNOHANG`: Returns immediately if no child state changes have occurred (used for background reaping).
- `WCONTINUED`: Reports status if a stopped child was resumed via `SIGCONT`.

---

## 8. Process Relationships

- **Shell Process**: Acts as the session leader / parent controller. Owns the controlling terminal when no foreground job is active.
- **Process Group Leader**: The first child process created in a command or pipeline. Its PID becomes the `PGID` for all pipeline children.
- **Job Hierarchy**: All processes in a pipeline (`cmd1 | cmd2 | cmd3`) share a single `PGID`, forming one logical job in the job table.

---

## 9. File-Descriptor Relationships

- The controlling terminal (`STDIN_FILENO = 0`) is inspected using `isatty()`.
- Terminal-control system calls (`tcsetpgrp`) are strictly guarded to operate only when standard input is connected to a TTY, ensuring flawless operation during scripted/non-interactive test runs.

---

## 10. Design Decisions

1. **Why treat entire pipelines as single jobs?**
   - In standard POSIX shells, a pipeline like `cat | grep pattern` is a single atomic task from the user's perspective. Grouping all pipeline PIDs under a single `Job` structure with a shared `PGID` ensures `fg`, `bg`, and Ctrl+Z operate on the whole pipeline rather than fragmented child processes.
2. **Why ignore `SIGTTOU` in the shell?**
   - When a background process calls `tcsetpgrp()`, the kernel sends `SIGTTOU` to the calling process group. Ignoring `SIGTTOU` in the shell prevents it from accidentally suspending itself during terminal ownership transitions.
3. **Why call `setpgid` in both parent and child?**
   - If only the child called `setpgid()`, the parent might call `tcsetpgrp()` or `waitpid()` before the child finished the call, causing `EACCES` or `EPERM`. If only the parent called `setpgid()`, the child might execute `execvp()` first. Symmetrical calls guarantee synchronization.

---

## 11. Error Handling

- **Invalid Job IDs**: `fg` and `bg` validate user arguments, checking for non-numeric input, out-of-range job numbers, or finished jobs, and emit descriptive error messages (`mini-shell: fg: <id>: no such job`).
- **`setpgid` Race Safety**: Non-fatal errors (such as `EACCES` when child has already called `execvp`) are handled cleanly without aborting.
- **Reaping and Zombie Prevention**: Non-blocking reaping loops before each prompt prevent zombie processes from accumulating.

---

## 12. Test Cases

Automated in `tests/run_tests.sh`:
- `test_jobs_empty`: Verifies `jobs` prints nothing when no active jobs exist.
- `test_jobs_listing`: Verifies `sleep 1 &` registers in `jobs` as `running`.
- `test_jobs_pipeline_single_job`: Verifies `sleep 1 | cat &` registers as a single logical job `[1]`.
- `test_fg_invalid_job`: Asserts descriptive error when invalid job ID is requested.
- `test_bg_invalid_job`: Asserts descriptive error when invalid job ID is requested.
- `test_fg_brings_background_job_to_foreground`: Tests foreground handoff and completion.
- `test_jobs_clean_after_fg_completion`: Verifies job table cleanup once a foreground job completes.

---

## 13. Edge Cases

- **Non-interactive Environments**: `tcsetpgrp` calls are bypassed when `isatty(STDIN_FILENO)` is false, avoiding `ENOTTY` errors in automated test pipelines.
- **Consecutive Stops & Resumes**: Handles repeated `fg` -> Ctrl+Z -> `bg` -> `fg` cycles without leaking processes or losing PGID associations.

---

## 14. Known Limitations

- Shell does not yet implement job control `%+` / `%-` current/previous job shortcuts (uses integer job IDs `%1`, `1`).
- Advanced shell job expansion syntax (`%string`) is not implemented.

---

## 15. Operating Systems Concepts Demonstrated

- **Process Groups & Sessions**: How kernels group processes for signal delivery and terminal access control.
- **Terminal Arbitration**: Controlling terminal driver semantics, foreground process groups, and `SIGTTOU` handling.
- **Signal-Driven State Transitions**: Suspending processes via `SIGTSTP`, resuming via `SIGCONT`, and asynchronous state monitoring via `waitpid(..., WUNTRACED | WCONTINUED)`.

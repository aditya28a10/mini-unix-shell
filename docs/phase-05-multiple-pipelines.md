# Phase 5 — Multiple Pipelines

## What I learned

Phase 4 implemented a single pipeline:

```text
command1 | command2
```

Phase 5 generalizes this to multiple commands:

```text
command1 | command2 | command3 | ... | commandN
```

For example:

```bash
ls | grep cpp | wc -l
```

The output of each command becomes the input of the next command.

---

## N Commands Require N-1 Pipes

The key rule for a pipeline is:

```text
N commands → N - 1 pipes
```

For:

```bash
ls | grep cpp | wc -l
```

there are three commands and two pipes:

```text
ls ──→ PIPE 1 ──→ grep cpp ──→ PIPE 2 ──→ wc -l
```

Each pipe connects the stdout of one command to the stdin of the next command.

---

## Pipeline Representation

Phase 4 used:

```cpp
struct Pipeline {
    Command first;
    Command second;
};
```

That could represent only two commands.

Phase 5 changes this to:

```cpp
struct Pipeline {
    std::vector<Command> commands;
};
```

Now the same structure can represent:

```text
A | B
A | B | C
A | B | C | D
```

This removes the two-command limitation and allows the executor to work with an arbitrary number of pipeline stages.

---

## Parsing Multiple Pipes

The parser now searches for every `|` instead of only the first one.

For:

```text
echo hello | grep hello | wc -l
```

the parser produces:

```text
commands[0] → echo hello
commands[1] → grep hello
commands[2] → wc -l
```

Empty commands are also detected later by the shell.

For example:

```text
echo hello || wc
```

produces an empty command between the two pipe operators and is rejected.

---

## Creating the Pipes

For `N` commands, the executor creates `N - 1` pipes.

The implementation stores them as:

```cpp
std::vector<std::array<int, 2>> pipes;
```

Each pipe contains:

```text
pipe[i][0] → read end
pipe[i][1] → write end
```

For three commands:

```text
Command 0 → Pipe 0 → Command 1 → Pipe 1 → Command 2
```

---

## Child Process Descriptor Setup

For command index `i`:

```text
if i > 0:
    stdin comes from pipe[i - 1]

if i < N - 1:
    stdout goes to pipe[i]
```

Therefore:

| Command | stdin | stdout |
|---|---|---|
| First | normal stdin | first pipe |
| Middle | previous pipe | next pipe |
| Last | previous pipe | normal stdout |

For:

```bash
ls | grep cpp | wc -l
```

the descriptor flow is:

```text
ls
 │ stdout
 ▼
Pipe 0
 │
 ▼
grep cpp
 │ stdout
 ▼
Pipe 1
 │
 ▼
wc -l
```

---

## `dup2()` in Multiple Pipelines

The same `dup2()` concept from Phase 3 and Phase 4 is reused.

For a command that is not the first:

```cpp
dup2(pipes[i - 1][0], STDIN_FILENO);
```

For a command that is not the last:

```cpp
dup2(pipes[i][1], STDOUT_FILENO);
```

This means the command itself still only sees normal `stdin` and `stdout`.

The shell prepares those descriptors before `execvp()`.

---

## Closing Pipe Descriptors

After the required `dup2()` operations, every child closes all original pipe descriptors.

The parent also closes all pipe descriptors after creating the children.

This is important because keeping unnecessary pipe ends open can prevent EOF from reaching a reader.

The intended structure is:

```text
Child i:
    connect required pipe ends
    close all original pipe descriptors
    execvp()

Parent:
    create all children
    close all pipe descriptors
    wait for children
```

---

## Creating the Child Processes

The executor creates one child for every command.

For:

```text
A | B | C
```

the shell creates:

```text
Child 1 → A
Child 2 → B
Child 3 → C
```

Each child gets a different position in the pipeline and therefore different stdin/stdout connections.

---

## Waiting for Children

After creating all children, the parent waits for every child using:

```cpp
waitpid(...)
```

The implementation records the status of each child but returns the status of the final command in the pipeline.

For example:

```text
A | B | C
```

the pipeline status is based on `C`.

---

## Error Handling

Invalid pipelines are rejected.

Examples:

```bash
| cat
cat |
cat || wc
cat | | wc
```

These contain an empty pipeline command.

A missing executable is also reported:

```bash
echo hello | nonexistent | wc -l
```

The corresponding child reports the `execvp()` error while the shell remains usable.

---

## Relationship With Earlier Phases

Phase 5 builds directly on the previous phases.

### Phase 3

Learned:

```text
open()
dup2()
close()
```

for connecting a process to files.

### Phase 4

Learned:

```text
pipe()
fork()
dup2()
close()
waitpid()
```

for connecting two processes.

### Phase 5

Generalizes the same mechanism:

```text
N commands
N - 1 pipes
N children
```

Each adjacent pair of commands is connected by one pipe.

---

## Example

For:

```bash
echo hello | cat | wc -w
```

the structure is:

```text
             Pipe 1              Pipe 2

echo ───────────────→ cat ───────────────→ wc
 stdout              stdin/stdout          stdin
```

The shell creates both pipes before creating the children.

Each child then connects its descriptors according to its position in the pipeline.

---

## Current Limitations

Phase 5 does not implement:

- background execution
- signal handling
- job control
- quoting
- command substitution
- full shell grammar
- combined pipeline and redirection semantics

These are intentionally left for later phases.

---

## Key Takeaway

> Multiple pipelines are an extension of the single-pipe mechanism: for `N` commands, the shell creates `N - 1` pipes and `N` child processes, connecting each command's standard input and output to the appropriate pipe ends.

The important OS idea is that the shell is not manually moving data between processes. It creates the processes and connects their file descriptors; the kernel manages the actual pipes and data transfer.


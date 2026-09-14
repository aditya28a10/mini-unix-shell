#ifndef MINI_SHELL_EXECUTOR_HPP
#define MINI_SHELL_EXECUTOR_HPP

#include "command.hpp"

namespace minishell {

// Executes an external command using fork(), execvp(), and waitpid().
// Returns the exit status of the child process, or -1 on system call failure.
int execute_command(const Command& cmd);

} // namespace minishell

#endif // MINI_SHELL_EXECUTOR_HPP

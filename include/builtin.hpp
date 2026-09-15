#ifndef MINI_SHELL_BUILTIN_HPP
#define MINI_SHELL_BUILTIN_HPP

#include <string>
#include "command.hpp"

namespace minishell {

// Checks whether a given command name is a supported shell built-in.
bool is_builtin(const std::string& name);

// Executes a shell built-in command in-process.
// Sets should_exit to true if the shell should terminate (e.g., 'exit').
// Returns the status code resulting from executing the built-in.
int execute_builtin(const Command& cmd, bool& should_exit, int last_status);

} // namespace minishell

#endif // MINI_SHELL_BUILTIN_HPP

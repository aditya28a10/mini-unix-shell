#ifndef MINI_SHELL_SHELL_HPP
#define MINI_SHELL_SHELL_HPP

namespace minishell {

class Shell {
public:
    Shell() = default;

    // Starts the shell Read-Eval-Print Loop (REPL).
    // Returns the exit code of the last executed command or 0 on clean exit.
    int run();
};

} // namespace minishell

#endif // MINI_SHELL_SHELL_HPP

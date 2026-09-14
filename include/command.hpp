#ifndef MINI_SHELL_COMMAND_HPP
#define MINI_SHELL_COMMAND_HPP

#include <string>
#include <vector>

namespace minishell {

// Represents a parsed shell command with its argument list.
struct Command {
    std::vector<std::string> args;

    bool empty() const {
        return args.empty();
    }

    const std::string& name() const {
        return args.front();
    }
};

} // namespace minishell

#endif // MINI_SHELL_COMMAND_HPP

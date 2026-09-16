#ifndef MINI_SHELL_COMMAND_HPP
#define MINI_SHELL_COMMAND_HPP

#include <string>
#include <vector>

namespace minishell {

enum class RedirectionType {
    Input,
    Output,
    Append
};

struct Redirection {
    RedirectionType type;
    std::string filename;
};

// Represents a parsed shell command.
struct Command {
    std::vector<std::string> args;
    std::vector<Redirection> redirections;

    bool empty() const {
        return args.empty();
    }

    const std::string& name() const {
        return args.front();
    }
};

} // namespace minishell

#endif // MINI_SHELL_COMMAND_HPP

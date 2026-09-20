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

// Represents one parsed shell command.
struct Command {
    std::vector<std::string> args;
    std::vector<Redirection> redirections;
    bool background = false;
    bool empty() const {
        return args.empty();
    }

    const std::string& name() const {
        return args.front();
    }
};

// Represents a pipeline containing one or more commands.
// Phase 5 supports multiple commands:
// command1 | command2 | ... | commandN
struct Pipeline {
    std::vector<Command> commands;

    bool empty() const {
        return commands.empty();
    }
};

} // namespace minishell

#endif // MINI_SHELL_COMMAND_HPP
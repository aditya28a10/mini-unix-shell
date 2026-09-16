#include "parser.hpp"

#include <sstream>
#include <string>

namespace minishell {

Command parse_command(const std::string& line) {
    Command cmd;
    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
        if (token == "<" || token == ">" || token == ">>") {
            std::string filename;

            if (!(iss >> filename)) {
                // Missing filename will be handled as an invalid
                // redirection by the executor.
                cmd.redirections.push_back({
                    token == "<" ? RedirectionType::Input
                                 : token == ">" ? RedirectionType::Output
                                                : RedirectionType::Append,
                    ""
                });
                continue;
            }

            RedirectionType type;

            if (token == "<") {
                type = RedirectionType::Input;
            } else if (token == ">") {
                type = RedirectionType::Output;
            } else {
                type = RedirectionType::Append;
            }

            cmd.redirections.push_back({type, filename});
        } else {
            cmd.args.push_back(token);
        }
    }

    return cmd;
}

} // namespace minishell

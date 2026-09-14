#include "parser.hpp"
#include <sstream>

namespace minishell {

Command parse_command(const std::string& line) {
    Command cmd;
    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
        cmd.args.push_back(token);
    }

    return cmd;
}

} // namespace minishell

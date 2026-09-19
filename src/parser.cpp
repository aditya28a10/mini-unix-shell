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
Pipeline parse_pipeline(const std::string& line) {
    Pipeline pipeline;

    std::size_t start = 0;

    while (start <= line.size()) {
        const std::size_t pipe_pos = line.find('|', start);

        const std::string command_part =
            line.substr(start, pipe_pos == std::string::npos
                                    ? std::string::npos
                                    : pipe_pos - start);

        pipeline.commands.push_back(parse_command(command_part));

        if (pipe_pos == std::string::npos) {
            break;
        }

        start = pipe_pos + 1;
    }

    return pipeline;
}

} // namespace minishell

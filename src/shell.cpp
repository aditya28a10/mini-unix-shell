#include "shell.hpp"
#include "parser.hpp"
#include "executor.hpp"
#include "builtin.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

namespace minishell {

int Shell::run() {
    std::string line;
    int last_status = 0;
    bool interactive = (isatty(STDIN_FILENO) != 0);

    while (true) {
        if (interactive) {
            std::cout << "mini-shell$ " << std::flush;
        }

        if (!std::getline(std::cin, line)) {
            // Reached End-Of-File (EOF / Ctrl+D)
            if (interactive) {
                std::cout << "\n";
            }
            break;
        }

        if (line.find('|') != std::string::npos) {
    Pipeline pipeline = parse_pipeline(line);

    bool invalid_pipeline = pipeline.commands.empty();

    for (const auto& command : pipeline.commands) {
        if (command.empty()) {
            invalid_pipeline = true;
            break;
        }
    }

    if (invalid_pipeline) {
        std::cerr << "mini-shell: invalid pipe\n";
        last_status = 2;
        continue;
    }

    last_status = execute_pipeline(pipeline);
    continue;
}

        Command cmd = parse_command(line);

        if (cmd.empty()) {
            continue;
        }

        if (is_builtin(cmd.name())) {
            bool should_exit = false;
            last_status = execute_builtin(cmd, should_exit, last_status);

            if (should_exit) {
                return last_status;
            }
        }
        else {
            last_status = execute_command(cmd);
        }
    }
    return last_status;
}

} // namespace minishell

#include "shell.hpp"
#include "parser.hpp"
#include "executor.hpp"
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

        Command cmd = parse_command(line);
        if (cmd.empty()) {
            continue;
        }

        last_status = execute_command(cmd);
    }

    return last_status;
}

} // namespace minishell

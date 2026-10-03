#include <iostream>
#include <string_view>
#include "version.hpp"
#include "shell.hpp"

namespace {

void print_help(std::string_view program_name) {
    std::cout << program_name << " - " << minishell::SHELL_DESCRIPTION << "\n\n"
              << "Usage:\n"
              << "  " << program_name << " [options]\n\n"
              << "Options:\n"
              << "  -h, --help       Display this help message and exit\n"
              << "  -v, --version    Display version information and exit\n\n"
              << "Features:\n"
              << "  - External command execution (fork, execvp, waitpid)\n"
              << "  - Built-in commands: cd, pwd, exit, help, jobs, fg, bg\n"
              << "  - I/O redirection: <, >, >>\n"
              << "  - Multi-stage pipelines: |\n"
              << "  - Background process execution: &\n"
              << "  - Process group and terminal job control (tcsetpgrp, SIGTSTP, SIGINT, SIGCHLD)\n";
}

void print_version() {
    std::cout << minishell::SHELL_NAME << " version " << minishell::SHELL_VERSION << "\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string_view arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            print_version();
            return 0;
        }

        std::cerr << "mini-shell: unrecognized option '" << arg << "'\n";
        std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
        return 1;
    }

    minishell::Shell shell;
    return shell.run();
}

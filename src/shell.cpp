#include "shell.hpp"
#include "parser.hpp"
#include "executor.hpp"
#include "builtin.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <csignal>

volatile sig_atomic_t g_sigint_received = 0;
volatile sig_atomic_t g_sigchld_received = 0;

void handle_sigint(int) {
	g_sigint_received = 1;
}

void handle_sigchld(int) {
	g_sigchld_received = 1;
}

namespace minishell {

int Shell::run() {
	struct sigaction sa {};
	sa.sa_handler = handle_sigint;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, nullptr) == -1) {
		perror("mini-shell: sigaction");
		return 1;
	}
	struct sigaction chld_sa {};
	chld_sa.sa_handler = handle_sigchld;
	sigemptyset(&chld_sa.sa_mask);
	chld_sa.sa_flags = 0;

	if (sigaction(SIGCHLD, &chld_sa, nullptr) == -1) {
		perror("mini-shell: sigaction SIGCHLD");
		return 1;
	}
	std::string line;
	int last_status = 0;
	bool interactive = (isatty(STDIN_FILENO) != 0);

	while (true) {
		if (g_sigchld_received) {
			g_sigchld_received = 0;
			reap_background_processes();
		}
		if (interactive) {
			std::cout << "mini-shell$ " << std::flush;
		}
		if (!std::getline(std::cin, line)) {
			if (g_sigint_received) {
				g_sigint_received = 0;
				std::cin.clear();

				if (interactive) {
					std::cout << "\n";
				}

				continue;
			}

			if (g_sigchld_received) {
				g_sigchld_received = 0;
				std::cin.clear();

				if (interactive) {
					std::cout << "\n";
				}

				reap_background_processes();
				continue;
			}

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

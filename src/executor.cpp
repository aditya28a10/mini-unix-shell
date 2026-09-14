#include "executor.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace minishell {

int execute_command(const Command& cmd) {
    if (cmd.empty()) {
        return 0;
    }

    // Convert std::vector<std::string> to null-terminated char* array required by execvp.
    std::vector<char*> c_argv;
    c_argv.reserve(cmd.args.size() + 1);

    for (const auto& arg : cmd.args) {
        c_argv.push_back(const_cast<char*>(arg.c_str()));
    }
    c_argv.push_back(nullptr);

    // Create a new child process
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failure (e.g., resource exhaustion: EAGAIN, ENOMEM)
        perror("mini-shell: fork");
        return -1;
    }

    if (pid == 0) {
        // --- Child Process ---
        // Replace child address space with the target executable.
        execvp(c_argv[0], c_argv.data());

        // execvp only returns if an error occurred.
        std::string err_prefix = "mini-shell: " + cmd.args[0];
        perror(err_prefix.c_str());

        // Terminate child immediately with POSIX exit codes without flushing parent buffers:
        // 127 = command not found (ENOENT)
        // 126 = command found but not executable (EACCES)
        // 1   = general execution error
        if (errno == ENOENT) {
            _exit(127);
        } else if (errno == EACCES) {
            _exit(126);
        } else {
            _exit(1);
        }
    }

    // --- Parent Process ---
    // Wait synchronously for child process to terminate and collect its exit status.
    int status = 0;
    pid_t wpid = waitpid(pid, &status, 0);

    if (wpid < 0) {
        perror("mini-shell: waitpid");
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return status;
}

} // namespace minishell

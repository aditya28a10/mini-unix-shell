#include "executor.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
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
    for (const Redirection& redirection : cmd.redirections) {
        int fd = -1;

        if (redirection.type == RedirectionType::Input) {
            fd = open(redirection.filename.c_str(), O_RDONLY);

            if (fd == -1) {
                perror("mini-shell: input redirection");
                _exit(1);
            }

            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("mini-shell: dup2");
                close(fd);
                _exit(1);
            }
        }
        else if (redirection.type == RedirectionType::Output) {
            fd = open(
                redirection.filename.c_str(),
                O_WRONLY | O_CREAT | O_TRUNC,
                0644
            );

            if (fd == -1) {
                perror("mini-shell: output redirection");
                _exit(1);
            }

            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("mini-shell: dup2");
                close(fd);
                _exit(1);
            }
        }
        else if (redirection.type == RedirectionType::Append) {
            fd = open(
                redirection.filename.c_str(),
                O_WRONLY | O_CREAT | O_APPEND,
                0644
            );

            if (fd == -1) {
                perror("mini-shell: append redirection");
                _exit(1);
            }

            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("mini-shell: dup2");
                close(fd);
                _exit(1);
            }
        }

        close(fd);
    }

    execvp(c_argv[0], c_argv.data());

    int exec_errno = errno;
    perror(("mini-shell: " + cmd.args[0]).c_str());

    if (exec_errno == ENOENT) {
        _exit(127);
    }
    else if (exec_errno == EACCES) {
        _exit(126);
    }
    else {
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

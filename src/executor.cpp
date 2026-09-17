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
int execute_pipeline(const Pipeline& pipeline) {
    if (pipeline.first.empty() || pipeline.second.empty()) {
        return 2;
    }

    int pipe_fds[2];

    if (pipe(pipe_fds) == -1) {
        perror("mini-shell: pipe");
        return -1;
    }

    std::vector<char*> first_argv;
    first_argv.reserve(pipeline.first.args.size() + 1);

    for (const auto& arg : pipeline.first.args) {
        first_argv.push_back(const_cast<char*>(arg.c_str()));
    }
    first_argv.push_back(nullptr);

    std::vector<char*> second_argv;
    second_argv.reserve(pipeline.second.args.size() + 1);

    for (const auto& arg : pipeline.second.args) {
        second_argv.push_back(const_cast<char*>(arg.c_str()));
    }
    second_argv.push_back(nullptr);

    pid_t first_pid = fork();

    if (first_pid < 0) {
        perror("mini-shell: fork");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }

    if (first_pid == 0) {
        // First child: stdout -> pipe
        if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
            perror("mini-shell: dup2");
            _exit(1);
        }

        close(pipe_fds[0]);
        close(pipe_fds[1]);

        execvp(first_argv[0], first_argv.data());

        int exec_errno = errno;
        perror(("mini-shell: " + pipeline.first.args[0]).c_str());

        if (exec_errno == ENOENT) {
            _exit(127);
        } else if (exec_errno == EACCES) {
            _exit(126);
        } else {
            _exit(1);
        }
    }

    pid_t second_pid = fork();

    if (second_pid < 0) {
        perror("mini-shell: fork");

        close(pipe_fds[0]);
        close(pipe_fds[1]);

        waitpid(first_pid, nullptr, 0);
        return -1;
    }

    if (second_pid == 0) {
        // Second child: stdin <- pipe
        if (dup2(pipe_fds[0], STDIN_FILENO) == -1) {
            perror("mini-shell: dup2");
            _exit(1);
        }

        close(pipe_fds[0]);
        close(pipe_fds[1]);

        execvp(second_argv[0], second_argv.data());

        int exec_errno = errno;
        perror(("mini-shell: " + pipeline.second.args[0]).c_str());

        if (exec_errno == ENOENT) {
            _exit(127);
        } else if (exec_errno == EACCES) {
            _exit(126);
        } else {
            _exit(1);
        }
    }

    // Parent no longer needs either end of the pipe.
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    int first_status = 0;
    int second_status = 0;

    if (waitpid(first_pid, &first_status, 0) == -1) {
        perror("mini-shell: waitpid");
        return -1;
    }

    if (waitpid(second_pid, &second_status, 0) == -1) {
        perror("mini-shell: waitpid");
        return -1;
    }

    if (WIFEXITED(second_status)) {
        return WEXITSTATUS(second_status);
    }

    if (WIFSIGNALED(second_status)) {
        return 128 + WTERMSIG(second_status);
    }

    return second_status;
}
} // namespace minishell

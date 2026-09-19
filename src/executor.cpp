#include "executor.hpp"
#include <array>
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
    if (pipeline.commands.empty()) {
        return 2;
    }

    const std::size_t command_count = pipeline.commands.size();

    if (command_count == 1) {
        return execute_command(pipeline.commands[0]);
    }

    std::vector<std::array<int, 2>> pipes(command_count - 1);

    for (std::size_t i = 0; i < pipes.size(); ++i) {
        if (pipe(pipes[i].data()) == -1) {
            perror("mini-shell: pipe");

            for (std::size_t j = 0; j < i; ++j) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return -1;
        }
    }

    std::vector<pid_t> pids;
    pids.reserve(command_count);

    for (std::size_t i = 0; i < command_count; ++i) {
        const Command& cmd = pipeline.commands[i];

        std::vector<char*> argv;
        argv.reserve(cmd.args.size() + 1);

        for (const auto& arg : cmd.args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        argv.push_back(nullptr);

        pid_t pid = fork();

        if (pid < 0) {
            perror("mini-shell: fork");

            for (const auto& pipe_fds : pipes) {
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }

            for (pid_t child_pid : pids) {
                waitpid(child_pid, nullptr, 0);
            }

            return -1;
        }

        if (pid == 0) {
            // Every command except the first reads from the previous pipe.
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("mini-shell: dup2");
                    _exit(1);
                }
            }

            // Every command except the last writes to the next pipe.
            if (i < command_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("mini-shell: dup2");
                    _exit(1);
                }
            }

            // The child no longer needs any original pipe descriptors.
            for (const auto& pipe_fds : pipes) {
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }

            execvp(argv[0], argv.data());

            int exec_errno = errno;
            perror(("mini-shell: " + cmd.args[0]).c_str());

            if (exec_errno == ENOENT) {
                _exit(127);
            } else if (exec_errno == EACCES) {
                _exit(126);
            } else {
                _exit(1);
            }
        }

        pids.push_back(pid);
    }

    // Parent no longer needs any pipe descriptors.
    for (const auto& pipe_fds : pipes) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
    }

    int last_status = 0;

    for (std::size_t i = 0; i < pids.size(); ++i) {
        int status = 0;

        if (waitpid(pids[i], &status, 0) == -1) {
            perror("mini-shell: waitpid");
            return -1;
        }

        if (i == pids.size() - 1) {
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_status = 128 + WTERMSIG(status);
            }
        }
    }

    return last_status;
}
} // namespace minishell

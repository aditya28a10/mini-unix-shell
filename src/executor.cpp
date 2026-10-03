#include "executor.hpp"
#include "job.hpp"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace minishell {

namespace {

std::string format_command_string(const Command& cmd) {
    std::string result;
    for (std::size_t i = 0; i < cmd.args.size(); ++i) {
        if (i > 0) {
            result += " ";
        }
        result += cmd.args[i];
    }
    for (const auto& redirection : cmd.redirections) {
        result += " ";
        if (redirection.type == RedirectionType::Input) {
            result += "< ";
        } else if (redirection.type == RedirectionType::Output) {
            result += "> ";
        } else if (redirection.type == RedirectionType::Append) {
            result += ">> ";
        }
        result += redirection.filename;
    }
    if (cmd.background) {
        result += " &";
    }
    return result;
}

std::string format_pipeline_string(const Pipeline& pipeline) {
    std::string result;
    for (std::size_t i = 0; i < pipeline.commands.size(); ++i) {
        if (i > 0) {
            result += " | ";
        }
        result += format_command_string(pipeline.commands[i]);
    }
    return result;
}

void reset_child_signals() {
    struct sigaction sa {};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTSTP, &sa, nullptr);
    sigaction(SIGTTIN, &sa, nullptr);
    sigaction(SIGTTOU, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);
    sigaction(SIGCHLD, &sa, nullptr);

    sigset_t mask;
    sigemptyset(&mask);
    sigprocmask(SIG_SETMASK, &mask, nullptr);
}

} // namespace

bool apply_redirections(const std::vector<Redirection>& redirections) {
    for (const auto& redirection : redirections) {
        if (redirection.filename.empty()) {
            std::cerr << "mini-shell: syntax error near unexpected token 'newline'\n";
            return false;
        }
        int fd = -1;

        if (redirection.type == RedirectionType::Input) {
            fd = open(redirection.filename.c_str(), O_RDONLY);
            if (fd == -1) {
                perror("mini-shell: input redirection");
                return false;
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("mini-shell: dup2");
                close(fd);
                return false;
            }
        } else if (redirection.type == RedirectionType::Output) {
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("mini-shell: output redirection");
                return false;
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("mini-shell: dup2");
                close(fd);
                return false;
            }
        } else if (redirection.type == RedirectionType::Append) {
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("mini-shell: append redirection");
                return false;
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("mini-shell: dup2");
                close(fd);
                return false;
            }
        }
        close(fd);
    }
    return true;
}

void reap_background_processes() {
    JobManager::instance().print_job_notifications();
}

int wait_for_job(Job* job) {
    if (!job) {
        return 0;
    }

    int last_status = 0;
    pid_t pgid = job->pgid;
    bool interactive = (isatty(STDIN_FILENO) != 0);

    while (!job->is_completed() && !job->is_stopped()) {
        int status = 0;
        pid_t wpid;
        do {
            wpid = waitpid(-pgid, &status, WUNTRACED);
        } while (wpid == -1 && errno == EINTR);

        if (wpid > 0) {
            JobManager::instance().update_process_status(wpid, status);
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_status = 128 + WTERMSIG(status);
            }
        } else if (wpid == -1) {
            if (errno == ECHILD) {
                break;
            }
            break;
        }
    }

    if (interactive) {
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }

    if (job->is_stopped()) {
        job->state = JobState::Stopped;
        job->notified = true;
        std::cout << "\n[" << job->id << "] stopped " << job->command << "\n";
        last_status = 128 + SIGTSTP;
    } else if (job->is_completed()) {
        job->notified = true;
        JobManager::instance().cleanup_jobs();
    }

    return last_status;
}

int execute_command(const Command& cmd) {
    if (cmd.empty()) {
        return 0;
    }

    std::vector<char*> c_argv;
    c_argv.reserve(cmd.args.size() + 1);

    for (const auto& arg : cmd.args) {
        c_argv.push_back(const_cast<char*>(arg.c_str()));
    }
    c_argv.push_back(nullptr);

    bool interactive = (isatty(STDIN_FILENO) != 0);
    std::string cmd_str = format_command_string(cmd);

    pid_t pid = fork();

    if (pid < 0) {
        perror("mini-shell: fork");
        return -1;
    }

    if (pid == 0) {
        // Child: set process group to its own PID
        setpgid(0, 0);

        if (!cmd.background && interactive) {
            tcsetpgrp(STDIN_FILENO, getpid());
        }

        reset_child_signals();

        if (!apply_redirections(cmd.redirections)) {
            _exit(1);
        }

        execvp(c_argv[0], c_argv.data());

        int exec_errno = errno;
        perror(("mini-shell: " + cmd.args[0]).c_str());

        if (exec_errno == ENOENT) {
            _exit(127);
        } else if (exec_errno == EACCES || exec_errno == EISDIR) {
            _exit(126);
        } else {
            _exit(1);
        }
    }

    // Parent: ensure process group is set
    setpgid(pid, pid);

    if (!cmd.background && interactive) {
        tcsetpgrp(STDIN_FILENO, pid);
    }

    if (cmd.background) {
        int jid = JobManager::instance().add_job(pid, {pid}, cmd_str, true, JobState::Running);
        std::cout << "[" << jid << "] " << pid << "\n";
        return 0;
    }

    int jid = JobManager::instance().add_job(pid, {pid}, cmd_str, false, JobState::Running);
    Job* job = JobManager::instance().get_job(jid);
    return wait_for_job(job);
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

    bool background = pipeline.commands.back().background;
    bool interactive = (isatty(STDIN_FILENO) != 0);
    std::string pipe_str = format_pipeline_string(pipeline);

    std::vector<pid_t> pids;
    pids.reserve(command_count);
    pid_t pgid = 0;

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
                kill(child_pid, SIGTERM);
                pid_t r;
                do {
                    r = waitpid(child_pid, nullptr, 0);
                } while (r == -1 && errno == EINTR);
            }

            return -1;
        }

        if (pid == 0) {
            // Child: set PGID
            if (i == 0) {
                setpgid(0, 0);
            } else {
                setpgid(0, pgid);
            }

            if (!background && interactive) {
                tcsetpgrp(STDIN_FILENO, (i == 0 ? getpid() : pgid));
            }

            reset_child_signals();

            // Set up pipeline input/output
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("mini-shell: dup2");
                    _exit(1);
                }
            }

            if (i < command_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("mini-shell: dup2");
                    _exit(1);
                }
            }

            for (const auto& pipe_fds : pipes) {
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }

            if (!apply_redirections(cmd.redirections)) {
                _exit(1);
            }

            execvp(argv[0], argv.data());

            int exec_errno = errno;
            perror(("mini-shell: " + cmd.args[0]).c_str());

            if (exec_errno == ENOENT) {
                _exit(127);
            } else if (exec_errno == EACCES || exec_errno == EISDIR) {
                _exit(126);
            } else {
                _exit(1);
            }
        }

        if (i == 0) {
            pgid = pid;
        }

        setpgid(pid, pgid);

        if (!background && interactive) {
            tcsetpgrp(STDIN_FILENO, pgid);
        }

        pids.push_back(pid);
    }

    for (const auto& pipe_fds : pipes) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
    }

    if (background) {
        int jid = JobManager::instance().add_job(pgid, pids, pipe_str, true, JobState::Running);
        std::cout << "[" << jid << "] " << pids.front() << "\n";
        return 0;
    }

    int jid = JobManager::instance().add_job(pgid, pids, pipe_str, false, JobState::Running);
    Job* job = JobManager::instance().get_job(jid);
    return wait_for_job(job);
}

} // namespace minishell

#include "builtin.hpp"
#include "job.hpp"
#include "executor.hpp"

#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

namespace minishell {

namespace {

int builtin_cd(const Command& cmd) {
    if (cmd.args.size() > 2) {
        std::cerr << "mini-shell: cd: too many arguments\n";
        return 1;
    }

    const char* target_path = nullptr;

    if (cmd.args.size() == 1) {
        target_path = std::getenv("HOME");
        if (target_path == nullptr || *target_path == '\0') {
            std::cerr << "mini-shell: cd: HOME not set\n";
            return 1;
        }
    } else {
        target_path = cmd.args[1].c_str();
    }

    if (chdir(target_path) != 0) {
        perror("mini-shell: cd");
        return 1;
    }

    return 0;
}

int builtin_pwd() {
    char cwd_buf[PATH_MAX];

    if (getcwd(cwd_buf, sizeof(cwd_buf)) == nullptr) {
        perror("mini-shell: pwd");
        return 1;
    }

    std::cout << cwd_buf << '\n';
    return 0;
}

int builtin_exit(const Command& cmd, bool& should_exit, int last_status) {
    if (cmd.args.size() > 2) {
        std::cerr << "mini-shell: exit: too many arguments\n";
        return 1;
    }

    if (cmd.args.size() == 1) {
        should_exit = true;
        return last_status;
    }

    char* endptr = nullptr;
    errno = 0;
    const long code = std::strtol(cmd.args[1].c_str(), &endptr, 10);

    if (errno == ERANGE || endptr == cmd.args[1].c_str() || *endptr != '\0') {
        std::cerr << "mini-shell: exit: " << cmd.args[1]
                  << ": numeric argument required\n";
        should_exit = true;
        return 2;
    }

    should_exit = true;
    return static_cast<int>(code);
}

int builtin_jobs() {
    JobManager::instance().print_jobs();
    return 0;
}

int builtin_fg(const Command& cmd) {
    if (cmd.args.size() > 2) {
        std::cerr << "mini-shell: fg: too many arguments\n";
        return 1;
    }

    Job* job = nullptr;

    if (cmd.args.size() == 1) {
        job = JobManager::instance().get_current_job();
    } else {
        std::string arg = cmd.args[1];
        if (!arg.empty() && arg[0] == '%') {
            arg = arg.substr(1);
        }
        char* endptr = nullptr;
        errno = 0;
        long id = std::strtol(arg.c_str(), &endptr, 10);
        if (errno != 0 || *endptr != '\0' || arg.empty()) {
            std::cerr << "mini-shell: fg: " << cmd.args[1] << ": no such job\n";
            return 1;
        }
        job = JobManager::instance().get_job(static_cast<int>(id));
    }

    if (!job) {
        std::cerr << "mini-shell: fg: " << (cmd.args.size() > 1 ? cmd.args[1] : "current") << ": no such job\n";
        return 1;
    }

    std::cout << job->command << "\n";

    if (isatty(STDIN_FILENO)) {
        tcsetpgrp(STDIN_FILENO, job->pgid);
    }

    if (job->state == JobState::Stopped) {
        for (auto& proc : job->processes) {
            proc.stopped = false;
        }
        job->state = JobState::Running;
        kill(-job->pgid, SIGCONT);
    }

    job->background = false;
    return wait_for_job(job);
}

int builtin_bg(const Command& cmd) {
    if (cmd.args.size() > 2) {
        std::cerr << "mini-shell: bg: too many arguments\n";
        return 1;
    }

    Job* job = nullptr;

    if (cmd.args.size() == 1) {
        job = JobManager::instance().get_most_recent_stopped_job();
        if (!job) {
            job = JobManager::instance().get_current_job();
        }
    } else {
        std::string arg = cmd.args[1];
        if (!arg.empty() && arg[0] == '%') {
            arg = arg.substr(1);
        }
        char* endptr = nullptr;
        errno = 0;
        long id = std::strtol(arg.c_str(), &endptr, 10);
        if (errno != 0 || *endptr != '\0' || arg.empty()) {
            std::cerr << "mini-shell: bg: " << cmd.args[1] << ": no such job\n";
            return 1;
        }
        job = JobManager::instance().get_job(static_cast<int>(id));
    }

    if (!job) {
        std::cerr << "mini-shell: bg: " << (cmd.args.size() > 1 ? cmd.args[1] : "current") << ": no such job\n";
        return 1;
    }

    if (job->state == JobState::Running) {
        std::cerr << "mini-shell: bg: job " << job->id << " already in background\n";
        return 0;
    }

    for (auto& proc : job->processes) {
        proc.stopped = false;
    }
    job->state = JobState::Running;
    job->background = true;

    kill(-job->pgid, SIGCONT);
    std::cout << "[" << job->id << "] " << job->command << " &\n";
    return 0;
}

int builtin_help() {
    std::cout << "mini-shell built-in commands:\n"
              << "  cd [dir]       Change the current working directory to [dir].\n"
              << "                 If [dir] is omitted, changes to $HOME.\n"
              << "  pwd            Print the current working directory.\n"
              << "  exit [n]       Exit the shell with status n (or last command status).\n"
              << "  jobs           List active and background jobs.\n"
              << "  fg [job_id]    Bring a background or stopped job to the foreground.\n"
              << "  bg [job_id]    Resume a stopped job in the background.\n"
              << "  help           Display information about built-in commands.\n\n"
              << "Built-in commands execute directly within the shell process.\n"
              << "External commands execute in child process groups via fork() and execvp().\n";
    return 0;
}

} // namespace

bool is_builtin(const std::string& name) {
    return name == "cd" || name == "pwd" || name == "exit" || name == "help" ||
           name == "jobs" || name == "fg" || name == "bg";
}

int execute_builtin(const Command& cmd, bool& should_exit, int last_status) {
    should_exit = false;

    if (cmd.name() == "cd") {
        return builtin_cd(cmd);
    }
    if (cmd.name() == "pwd") {
        return builtin_pwd();
    }
    if (cmd.name() == "exit") {
        return builtin_exit(cmd, should_exit, last_status);
    }
    if (cmd.name() == "jobs") {
        return builtin_jobs();
    }
    if (cmd.name() == "fg") {
        return builtin_fg(cmd);
    }
    if (cmd.name() == "bg") {
        return builtin_bg(cmd);
    }
    if (cmd.name() == "help") {
        return builtin_help();
    }

    return 0;
}

} // namespace minishell

#include "job.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cerrno>

namespace minishell {

JobManager& JobManager::instance() {
    static JobManager mgr;
    return mgr;
}

int JobManager::add_job(pid_t pgid, const std::vector<pid_t>& pids, const std::string& command, bool background, JobState state) {
    Job job;
    job.id = next_job_id_++;
    job.pgid = pgid;
    job.command = command;
    job.background = background;
    job.state = state;
    job.notified = false;

    for (pid_t pid : pids) {
        ProcessInfo proc;
        proc.pid = pid;
        proc.completed = false;
        proc.stopped = (state == JobState::Stopped);
        job.processes.push_back(proc);
    }

    jobs_.push_back(job);
    return job.id;
}

Job* JobManager::get_job(int job_id) {
    for (auto& job : jobs_) {
        if (job.id == job_id) {
            return &job;
        }
    }
    return nullptr;
}

Job* JobManager::get_job_by_pgid(pid_t pgid) {
    for (auto& job : jobs_) {
        if (job.pgid == pgid) {
            return &job;
        }
    }
    return nullptr;
}

Job* JobManager::get_job_by_pid(pid_t pid) {
    for (auto& job : jobs_) {
        for (const auto& proc : job.processes) {
            if (proc.pid == pid) {
                return &job;
            }
        }
    }
    return nullptr;
}

Job* JobManager::get_current_job() {
    // Return the most recently added active job (highest ID that is not completed)
    for (auto it = jobs_.rbegin(); it != jobs_.rend(); ++it) {
        if (!it->is_completed()) {
            return &(*it);
        }
    }
    return nullptr;
}

Job* JobManager::get_most_recent_stopped_job() {
    for (auto it = jobs_.rbegin(); it != jobs_.rend(); ++it) {
        if (it->is_stopped()) {
            return &(*it);
        }
    }
    return nullptr;
}

bool JobManager::update_process_status(pid_t pid, int status) {
    for (auto& job : jobs_) {
        for (auto& proc : job.processes) {
            if (proc.pid == pid) {
                proc.status = status;
                if (WIFEXITED(status)) {
                    proc.completed = true;
                    proc.stopped = false;
                } else if (WIFSIGNALED(status)) {
                    proc.completed = true;
                    proc.stopped = false;
                } else if (WIFSTOPPED(status)) {
                    proc.stopped = true;
                } else if (WIFCONTINUED(status)) {
                    proc.stopped = false;
                }

                if (job.is_completed()) {
                    job.state = JobState::Done;
                } else if (job.is_stopped()) {
                    job.state = JobState::Stopped;
                } else {
                    job.state = JobState::Running;
                }
                return true;
            }
        }
    }
    return false;
}

void JobManager::reap_and_update() {
    int status = 0;
    while (true) {
        pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid > 0) {
            update_process_status(pid, status);
        } else if (pid == 0) {
            break;
        } else {
            if (errno == ECHILD) {
                break;
            }
            break;
        }
    }
}

void JobManager::print_jobs() {
    reap_and_update();

    for (auto& job : jobs_) {
        if (job.is_completed()) {
            std::cout << "[" << job.id << "] done " << job.command << "\n";
            job.notified = true;
        } else if (job.is_stopped()) {
            std::cout << "[" << job.id << "] stopped " << job.command << "\n";
        } else {
            std::cout << "[" << job.id << "] running " << job.command << "\n";
        }
    }

    cleanup_jobs();
}

void JobManager::cleanup_jobs() {
    jobs_.erase(
        std::remove_if(jobs_.begin(), jobs_.end(),
            [](const Job& job) {
                return job.is_completed() && job.notified;
            }),
        jobs_.end());
}

void JobManager::print_job_notifications() {
    reap_and_update();

    for (auto& job : jobs_) {
        if (job.background && !job.notified) {
            if (job.is_completed()) {
                std::cout << "[" << job.id << "] done " << job.command << "\n";
                job.notified = true;
            } else if (job.is_stopped()) {
                std::cout << "[" << job.id << "] stopped " << job.command << "\n";
                job.notified = true;
            }
        }
    }

    cleanup_jobs();
}

void JobManager::remove_job(int job_id) {
    jobs_.erase(
        std::remove_if(jobs_.begin(), jobs_.end(),
            [job_id](const Job& job) {
                return job.id == job_id;
            }),
        jobs_.end());
}

void JobManager::clear() {
    jobs_.clear();
    next_job_id_ = 1;
}

} // namespace minishell

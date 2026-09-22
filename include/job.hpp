#ifndef MINI_SHELL_JOB_HPP
#define MINI_SHELL_JOB_HPP

#include <string>
#include <vector>
#include <sys/types.h>

namespace minishell {

enum class JobState {
    Running,
    Stopped,
    Done,
    Terminated
};

struct ProcessInfo {
    pid_t pid = 0;
    bool completed = false;
    bool stopped = false;
    int status = 0;
};

struct Job {
    int id = 0;                         // Shell job ID (1, 2, 3...)
    pid_t pgid = 0;                     // Process Group ID
    std::vector<ProcessInfo> processes; // Processes belonging to this job
    std::string command;                // Formatted command string
    JobState state = JobState::Running;
    bool background = false;            // Whether job runs in background
    bool notified = false;              // Whether status change was reported

    bool is_completed() const {
        if (processes.empty()) return true;
        for (const auto& proc : processes) {
            if (!proc.completed) return false;
        }
        return true;
    }

    bool is_stopped() const {
        if (is_completed()) return false;
        for (const auto& proc : processes) {
            if (!proc.completed && !proc.stopped) return false;
        }
        return true;
    }
};

class JobManager {
public:
    static JobManager& instance();

    int add_job(pid_t pgid, const std::vector<pid_t>& pids, const std::string& command, bool background, JobState state = JobState::Running);
    Job* get_job(int job_id);
    Job* get_job_by_pgid(pid_t pgid);
    Job* get_job_by_pid(pid_t pid);
    Job* get_current_job();
    Job* get_most_recent_stopped_job();

    const std::vector<Job>& get_jobs() const { return jobs_; }
    std::vector<Job>& get_jobs() { return jobs_; }

    bool update_process_status(pid_t pid, int status);
    void reap_and_update();
    void print_jobs();
    void cleanup_jobs();
    void print_job_notifications();
    void remove_job(int job_id);
    void clear();

private:
    JobManager() = default;
    std::vector<Job> jobs_;
    int next_job_id_ = 1;
};

} // namespace minishell

#endif // MINI_SHELL_JOB_HPP

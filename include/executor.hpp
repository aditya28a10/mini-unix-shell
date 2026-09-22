#ifndef MINI_SHELL_EXECUTOR_HPP
#define MINI_SHELL_EXECUTOR_HPP

#include "command.hpp"
#include "job.hpp"

namespace minishell {

// Executes a single external command.
int execute_command(const Command& cmd);

// Executes a multi-stage pipeline of commands.
int execute_pipeline(const Pipeline& pipeline);

// Waits for a job to complete or stop in the foreground.
int wait_for_job(Job* job);

// Reaps any terminated or stopped background processes.
void reap_background_processes();

} // namespace minishell

#endif // MINI_SHELL_EXECUTOR_HPP

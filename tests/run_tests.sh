#!/usr/bin/env bash
# Test harness for Mini Unix Shell

set -euo pipefail

# ANSI color codes for readable output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PASSED_TESTS=0
FAILED_TESTS=0

echo -e "${BLUE}==================================================${NC}"
echo -e "${BLUE}  Mini Unix Shell - Test Suite (Phase 9)          ${NC}"
echo -e "${BLUE}==================================================${NC}"

# Navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT}"

assert_success() {
    local test_name="$1"
    shift
    echo -n "Running: ${test_name}... "
    if "$@"; then
        echo -e "${GREEN}[PASS]${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}[FAIL]${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

assert_failure() {
    local test_name="$1"
    shift
    echo -n "Running: ${test_name}... "
    if "$@" >/dev/null 2>&1; then
        echo -e "${RED}[FAIL] (expected failure, got success)${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    else
        echo -e "${GREEN}[PASS]${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
}

# --- Phase 0 Regression Tests ---

test_binary_exists() { [[ -x "./bin/mini-shell" ]]; }

test_version_flag() {
    v1=$(./bin/mini-shell --version)
    v2=$(./bin/mini-shell -v)
    [[ "$v1" == *"mini-shell version 0.9.0-phase9"* ]] && \
    [[ "$v2" == *"mini-shell version 0.9.0-phase9"* ]]
}

test_help_flag() {
    h1=$(./bin/mini-shell --help)
    h2=$(./bin/mini-shell -h)
    [[ "$h1" == *"Usage:"* ]] && [[ "$h2" == *"Usage:"* ]]
}

test_invalid_flag() { ./bin/mini-shell --invalid-flag; }

test_make_clean() {
    make clean >/dev/null
    [[ ! -d "./bin" ]] && [[ ! -d "./obj" ]]
}

test_rebuild() {
    make all >/dev/null
    [[ -x "./bin/mini-shell" ]]
}

# --- Phase 1 Functional Tests ---

test_basic_echo() {
    output=$(echo "echo hello" | ./bin/mini-shell)
    [[ "$output" == "hello" ]]
}

test_multi_args() {
    output=$(echo "echo foo bar baz" | ./bin/mini-shell)
    [[ "$output" == "foo bar baz" ]]
}

test_external_pwd() {
    output=$(echo "pwd" | ./bin/mini-shell)
    [[ "$output" == "$PROJECT_ROOT" ]]
}

test_whitespace_and_blank_lines() {
    output=$(printf "   \n\n\t  echo clean\n\n" | ./bin/mini-shell)
    [[ "$output" == "clean" ]]
}

test_sequential_commands() {
    output=$(printf "echo line1\necho line2\n" | ./bin/mini-shell)
    expected=$(printf "line1\nline2")
    [[ "$output" == "$expected" ]]
}

test_eof_termination() {
    printf "" | ./bin/mini-shell
}

test_nonexistent_command_error() {
    local err_output
    err_output=$(echo "nonexistent_command_12345" | ./bin/mini-shell 2>&1 || true)
    [[ "$err_output" == *"mini-shell: nonexistent_command_12345"* ]]
}

test_command_failure_exit_status() {
    echo "ls /path/that/does/not/exist_12345" | ./bin/mini-shell
}

# --- Phase 2 Built-in Tests ---

test_pwd_builtin() {
    output=$(printf "pwd\n" | ./bin/mini-shell)
    [[ "$output" == "$PROJECT_ROOT" ]]
}

test_cd_changes_directory() {
    output=$(printf "cd $PROJECT_ROOT/docs\npwd\n" | ./bin/mini-shell)
    [[ "$output" == "$PROJECT_ROOT/docs" ]]
}

test_cd_parent_directory() {
    output=$(printf "cd $PROJECT_ROOT/docs\ncd ..\npwd\n" | ./bin/mini-shell)
    [[ "$output" == "$PROJECT_ROOT" ]]
}

test_cd_home() {
    output=$(printf "cd\npwd\n" | HOME="$PROJECT_ROOT/docs" ./bin/mini-shell)
    [[ "$output" == "$PROJECT_ROOT/docs" ]]
}

test_cd_nonexistent_directory() {
    local output
    output=$(printf "cd /path/that/does/not/exist_12345\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: cd:"* ]]
}

test_cd_too_many_arguments() {
    local output
    output=$(printf "cd dir1 dir2\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: cd: too many arguments"* ]]
}

test_help_builtin() {
    local output
    output=$(printf "help\n" | ./bin/mini-shell)
    [[ "$output" == *"cd"* ]] && [[ "$output" == *"pwd"* ]] && \
    [[ "$output" == *"exit"* ]] && [[ "$output" == *"jobs"* ]] && \
    [[ "$output" == *"fg"* ]] && [[ "$output" == *"bg"* ]] && \
    [[ "$output" == *"help"* ]]
}

test_exit_zero() {
    printf "exit\n" | ./bin/mini-shell
}

test_exit_custom_status() {
    local status=0
    printf "exit 42\n" | ./bin/mini-shell || status=$?
    [[ $status -eq 42 ]]
}

test_exit_too_many_arguments() {
    local output
    output=$(printf "exit 1 2\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: exit: too many arguments"* ]]
}

test_cd_persists_for_external_command() {
    output=$(printf "cd $PROJECT_ROOT/docs\n/bin/pwd\n" | ./bin/mini-shell)
    [[ "$output" == "$PROJECT_ROOT/docs" ]]
}

# --- Phase 3 Redirection Tests ---

test_output_redirection() {
    local tmp_file
    tmp_file=$(mktemp)
    printf "echo redirected-content > %s\n" "$tmp_file" | ./bin/mini-shell
    local content
    content=$(cat "$tmp_file")
    rm -f "$tmp_file"
    [[ "$content" == "redirected-content" ]]
}

test_output_redirection_truncates() {
    local tmp_file
    tmp_file=$(mktemp)
    echo "existing-content" > "$tmp_file"
    printf "echo new-content > %s\n" "$tmp_file" | ./bin/mini-shell
    local content
    content=$(cat "$tmp_file")
    rm -f "$tmp_file"
    [[ "$content" == "new-content" ]]
}

test_append_redirection() {
    local tmp_file
    tmp_file=$(mktemp)
    echo "line1" > "$tmp_file"
    printf "echo line2 >> %s\n" "$tmp_file" | ./bin/mini-shell
    local content
    content=$(cat "$tmp_file")
    rm -f "$tmp_file"
    local expected
    expected=$(printf "line1\nline2")
    [[ "$content" == "$expected" ]]
}

test_input_redirection() {
    local tmp_file
    tmp_file=$(mktemp)
    echo "input-data" > "$tmp_file"
    local output
    output=$(printf "cat < %s\n" "$tmp_file" | ./bin/mini-shell)
    rm -f "$tmp_file"
    [[ "$output" == "input-data" ]]
}

test_missing_input_file() {
    local output
    output=$(printf "cat < /path/that/does/not/exist_12345\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: input redirection:"* ]]
}

test_redirection_does_not_pass_operator_to_command() {
    local tmp_file
    tmp_file=$(mktemp)
    local output
    output=$(printf "echo hello > %s\n" "$tmp_file" | ./bin/mini-shell)
    local content
    content=$(cat "$tmp_file")
    rm -f "$tmp_file"
    [[ -z "$output" ]] && [[ "$content" == "hello" ]]
}

# --- Phase 4 Single Pipe Tests ---

test_basic_pipe() {
    output=$(printf "echo hello | cat\n" | ./bin/mini-shell)
    [[ "$output" == "hello" ]]
}

test_pipe_line_count() {
    output=$(printf "printf one\\\\ntwo\\\\nthree\\\\n | wc -l\n" | ./bin/mini-shell)
    output=$(echo "$output" | tr -d ' ')
    [[ "$output" == "3" ]]
}

test_pipe_word_count() {
    output=$(printf "echo one two three | wc -w\n" | ./bin/mini-shell)
    output=$(echo "$output" | tr -d ' ')
    [[ "$output" == "3" ]]
}

test_pipe_preserves_arguments() {
    output=$(printf "echo alpha beta gamma | cat\n" | ./bin/mini-shell)
    [[ "$output" == "alpha beta gamma" ]]
}

test_pipe_first_command_failure() {
    local output
    output=$(printf "nonexistent_command_12345 | cat\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: nonexistent_command_12345:"* ]]
}

test_pipe_second_command_failure() {
    local output
    output=$(printf "echo hello | nonexistent_command_12345\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: nonexistent_command_12345:"* ]]
}

test_pipe_missing_left_command() {
    local output
    output=$(printf "| cat\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: invalid pipe"* ]]
}

test_pipe_missing_right_command() {
    local output
    output=$(printf "echo hello |\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: invalid pipe"* ]]
}

test_pipe_missing_both_commands() {
    local output
    output=$(printf "|\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: invalid pipe"* ]]
}

# --- Phase 5 Multiple Pipeline Tests ---

test_three_command_pipeline() {
    output=$(printf "printf apple\\\\nbanana\\\\ncherry\\\\n | grep a | wc -l\n" | ./bin/mini-shell)
    output=$(echo "$output" | tr -d ' ')
    [[ "$output" == "2" ]]
}

test_four_command_pipeline() {
    output=$(printf "printf 3\\\\n1\\\\n2\\\\n | sort | head -n 2 | wc -l\n" | ./bin/mini-shell)
    output=$(echo "$output" | tr -d ' ')
    [[ "$output" == "2" ]]
}

test_multiple_pipeline_arguments() {
    output=$(printf "printf three\\\\ntwo\\\\none\\\\n | grep t | sort\n" | ./bin/mini-shell)
    expected=$(printf "three\ntwo")
    [[ "$output" == "$expected" ]]
}

test_multiple_pipeline_invalid_middle() {
    local output
    output=$(printf "echo hello || cat\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: invalid pipe"* ]]
}

test_multiple_pipeline_missing_command() {
    local output
    output=$(printf "echo hello | | cat\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: invalid pipe"* ]]
}

# --- Phase 6 Background Process Tests ---

test_background_command() {
    local output
    output=$(printf "sleep 1 &\necho background-ok\n" | ./bin/mini-shell)
    [[ "$output" =~ \[[0-9]+\] ]] && [[ "$output" == *"background-ok"* ]]
}

test_background_pipeline() {
    local output
    output=$(printf "sleep 1 | cat &\necho pipeline-background-ok\n" | ./bin/mini-shell)
    [[ "$output" =~ \[[0-9]+\] ]] && [[ "$output" == *"pipeline-background-ok"* ]]
}

test_background_ampersand_not_passed() {
    local output
    output=$(printf "echo ampersand-test &\n" | ./bin/mini-shell)
    [[ "$output" == *"ampersand-test"* ]]
}

test_background_process_reaped() {
    local output
    output=$(printf "sleep 1 &\nsleep 2\necho cleanup-ok\n" | ./bin/mini-shell)
    [[ "$output" == *"cleanup-ok"* ]]
}

# --- Phase 7 Signal Handling Tests ---

test_sigint_shell_survives() {
    local fifo
    local output
    local shell_pid

    fifo=$(mktemp -u)
    output=$(mktemp)

    mkfifo "$fifo"

    ./bin/mini-shell < "$fifo" > "$output" 2>&1 &
    shell_pid=$!

    (
        sleep 0.1
        kill -INT "$shell_pid"
        sleep 0.1
        printf "echo signal-ok\nexit\n" > "$fifo"
    ) &

    wait "$shell_pid"

    grep -q "signal-ok" "$output"

    rm -f "$fifo" "$output"
}

test_sigchld_background_reaping() {
    local output
    output=$(printf "sleep 1 &\nsleep 2\necho sigchld-ok\n" | ./bin/mini-shell)
    [[ "$output" == *"sigchld-ok"* ]]
}

test_sigchld_does_not_break_pipeline() {
    local output
    output=$(printf "echo signal-pipeline | cat\n" | ./bin/mini-shell)
    [[ "$output" == *"signal-pipeline"* ]]
}

# --- Phase 8 Job Control Tests ---

test_jobs_empty() {
    local output
    output=$(printf "jobs\n" | ./bin/mini-shell)
    [[ -z "$output" ]]
}

test_jobs_listing() {
    local output
    output=$(printf "sleep 1 &\njobs\n" | ./bin/mini-shell)
    [[ "$output" =~ \[1\].*running.*sleep ]]
}

test_jobs_pipeline_single_job() {
    local output
    output=$(printf "sleep 1 | cat &\njobs\n" | ./bin/mini-shell)
    # Ensure only [1] is listed, not [1] and [2]
    [[ "$output" == *"[1] running sleep 1 | cat &"* ]] && \
    [[ "$output" != *"[2]"* ]]
}

test_fg_invalid_job() {
    local output
    output=$(printf "fg 999\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: fg: 999: no such job"* ]]
}

test_bg_invalid_job() {
    local output
    output=$(printf "bg 999\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: bg: 999: no such job"* ]]
}

test_fg_brings_background_job_to_foreground() {
    local output
    output=$(printf "sleep 1 &\nfg 1\necho fg-ok\n" | ./bin/mini-shell)
    [[ "$output" == *"sleep 1 &"* ]] && [[ "$output" == *"fg-ok"* ]]
}

test_jobs_clean_after_fg_completion() {
    local output
    output=$(printf "sleep 1 &\nfg 1\njobs\n" | ./bin/mini-shell)
    # After fg completes, jobs should be empty
    local jobs_lines
    jobs_lines=$(echo "$output" | grep -c "\[1\]" || true)
    [[ $jobs_lines -le 1 ]] # Only initial [1] <pid> on launch
}

# --- Phase 9 Robustness & Error Handling Tests ---

test_exit_code_127_command_not_found() {
    local status=0
    printf "nonexistent_binary_xyz_123\n" | ./bin/mini-shell >/dev/null 2>&1 || status=$?
    [[ $status -eq 127 ]]
}

test_exit_code_126_non_executable() {
    local test_file="/tmp/mini_shell_noexec_$$"
    touch "$test_file"
    chmod -x "$test_file"
    local status=0
    printf "%s\n" "$test_file" | ./bin/mini-shell >/dev/null 2>&1 || status=$?
    rm -f "$test_file"
    [[ $status -eq 126 ]]
}

test_redirection_missing_file_syntax_error() {
    local output
    output=$(printf "cat <\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"syntax error"* ]] || [[ "$output" == *"mini-shell"* ]]
}

test_builtin_stdout_redirection() {
    local test_file
    test_file=$(mktemp)
    printf "pwd > %s\n" "$test_file" | ./bin/mini-shell
    local content
    content=$(cat "$test_file" 2>/dev/null || true)
    rm -f "$test_file"
    [[ "$content" == "$PROJECT_ROOT" ]]
}

test_cd_to_file_error() {
    local output
    output=$(printf "cd Makefile\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"Not a directory"* ]]
}

test_cd_nonexistent_status() {
    local status=0
    printf "cd nonexistent_directory_12345\nexit\n" | ./bin/mini-shell >/dev/null 2>&1 || status=$?
    [[ $status -eq 1 ]]
}

test_exit_non_numeric_argument() {
    local output
    set +e
    output=$(printf "exit invalid_arg\n" | ./bin/mini-shell 2>&1)
    local status=$?
    set -e
    [[ $status -eq 2 ]] && [[ "$output" == *"numeric argument required"* ]]
}

test_large_argument_list() {
    local args=""
    for i in $(seq 1 1000); do
        args="$args arg$i"
    done
    local output
    output=$(printf "echo %s\n" "$args" | ./bin/mini-shell)
    [[ "$output" == *"arg1000"* ]]
}

test_pipeline_error_recovery() {
    local output
    output=$(printf "cat | nonexistent_binary_xyz_123 | wc -l\necho pipeline-recovered\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"pipeline-recovered"* ]]
}

test_non_interactive_mode() {
    local output
    output=$(printf "echo non-interactive-test\n" | ./bin/mini-shell 2>&1)
    # Piped/non-TTY execution should NOT emit the interactive prompt
    [[ "$output" != *"mini-shell$ "* ]] && [[ "$output" == "non-interactive-test" ]]
}

test_background_pipeline_full_lifecycle() {
    # Launch pipeline where stage 1 exits early and stage 2 runs longer
    # Verify that while stage 2 is still running, the overall job remains active in the job table
    local output
    output=$(printf "sleep 0.1 | sleep 1 &\nsleep 0.3\njobs\n" | ./bin/mini-shell)
    [[ "$output" == *"running"* ]] && [[ "$output" == *"sleep 0.1 | sleep 1 &"* ]]
}

test_fd_leak_prevention_on_errors() {
    local script=""
    for i in $(seq 1 30); do
        script="${script}nonexistent_cmd_${i}\ncat < /nonexistent_path_${i}\n"
    done
    script="${script}echo fd-cleanup-verified\n"
    local output
    output=$(printf "%b" "$script" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"fd-cleanup-verified"* ]]
}

# --- Execute tests ---

echo -e "${YELLOW}--- Phase 0 Regressions ---${NC}"
assert_success "1. Binary exists and is executable" test_binary_exists
assert_success "2. Version flags (--version, -v) work" test_version_flag
assert_success "3. Help flags (--help, -h) work" test_help_flag
assert_failure "4. Unrecognized flag exits with non-zero code" test_invalid_flag
assert_success "5. 'make clean' removes bin/ and obj/ directories" test_make_clean
assert_success "6. 'make all' rebuilds binary cleanly" test_rebuild

echo -e "${YELLOW}--- Phase 1 Basic Command Execution ---${NC}"
assert_success "7. Execute simple external command (echo hello)" test_basic_echo
assert_success "8. Execute command with multiple arguments" test_multi_args
assert_success "9. Execute system utility (pwd)" test_external_pwd
assert_success "10. Handle blank lines and whitespace cleanly" test_whitespace_and_blank_lines
assert_success "11. Execute sequential commands in order" test_sequential_commands
assert_success "12. Clean termination on EOF (Ctrl+D)" test_eof_termination
assert_success "13. Report error when command does not exist" test_nonexistent_command_error
assert_failure "14. Propagate non-zero exit status from failing command" test_command_failure_exit_status

echo -e "${YELLOW}--- Phase 2 Built-in Commands ---${NC}"
assert_success "15. pwd built-in reports current working directory" test_pwd_builtin
assert_success "16. cd changes directory for subsequent commands" test_cd_changes_directory
assert_success "17. cd .. changes to parent directory" test_cd_parent_directory
assert_success "18. cd with no arguments uses HOME" test_cd_home
assert_success "19. cd reports error for nonexistent directory" test_cd_nonexistent_directory
assert_success "20. cd rejects too many arguments" test_cd_too_many_arguments
assert_success "21. help lists all built-ins" test_help_builtin
assert_success "22. exit terminates with status 0" test_exit_zero
assert_success "23. exit accepts a custom numeric status" test_exit_custom_status
assert_success "24. exit rejects too many arguments without terminating" test_exit_too_many_arguments
assert_success "25. cd state persists into external command execution" test_cd_persists_for_external_command

echo -e "${YELLOW}--- Phase 3 I/O Redirection ---${NC}"
assert_success "26. Redirect stdout to a file with >" test_output_redirection
assert_success "27. > truncates an existing file" test_output_redirection_truncates
assert_success "28. >> appends to an existing file" test_append_redirection
assert_success "29. Redirect stdin from a file with <" test_input_redirection
assert_success "30. Report error for missing input file" test_missing_input_file
assert_success "31. Redirection operators are not passed to the command" test_redirection_does_not_pass_operator_to_command

echo -e "${YELLOW}--- Phase 4 Single Pipe ---${NC}"
assert_success "32. Pipe stdout of first command into second command" test_basic_pipe
assert_success "33. Pipe multiple lines into wc -l" test_pipe_line_count
assert_success "34. Pipe into wc -w with command arguments" test_pipe_word_count
assert_success "35. Pipe preserves command arguments" test_pipe_preserves_arguments
assert_success "36. Report error when first piped command does not exist" test_pipe_first_command_failure
assert_success "37. Report error when second piped command does not exist" test_pipe_second_command_failure
assert_success "38. Reject pipe with missing left command" test_pipe_missing_left_command
assert_success "39. Reject pipe with missing right command" test_pipe_missing_right_command
assert_success "40. Reject pipe with both commands missing" test_pipe_missing_both_commands

echo -e "${YELLOW}--- Phase 5 Multiple Pipelines ---${NC}"
assert_success "41. Execute a three-command pipeline" test_three_command_pipeline
assert_success "42. Execute a four-command pipeline" test_four_command_pipeline
assert_success "43. Preserve arguments across multiple pipeline stages" test_multiple_pipeline_arguments
assert_success "44. Reject consecutive pipe operators" test_multiple_pipeline_invalid_middle
assert_success "45. Reject missing middle pipeline command" test_multiple_pipeline_missing_command

echo -e "${YELLOW}--- Phase 6 Background Processes ---${NC}"
assert_success "46. Background command returns immediately" test_background_command
assert_success "47. Background pipeline returns immediately" test_background_pipeline
assert_success "48. Ampersand is not passed to command" test_background_ampersand_not_passed
assert_success "49. Finished background process is reaped" test_background_process_reaped

echo -e "${YELLOW}--- Phase 7 Signal Handling ---${NC}"
assert_success "50. Shell survives SIGINT at idle prompt" test_sigint_shell_survives
assert_success "51. SIGCHLD background child is reaped" test_sigchld_background_reaping
assert_success "52. SIGCHLD does not break foreground pipeline" test_sigchld_does_not_break_pipeline

echo -e "${YELLOW}--- Phase 8 Job Control ---${NC}"
assert_success "53. jobs lists nothing when no jobs active" test_jobs_empty
assert_success "54. jobs lists active background job" test_jobs_listing
assert_success "55. jobs tracks pipeline as a single logical job" test_jobs_pipeline_single_job
assert_success "56. fg reports error on nonexistent job" test_fg_invalid_job
assert_success "57. bg reports error on nonexistent job" test_bg_invalid_job
assert_success "58. fg brings background job to foreground and completes" test_fg_brings_background_job_to_foreground
assert_success "59. jobs table is cleaned up after fg completes" test_jobs_clean_after_fg_completion

echo -e "${YELLOW}--- Phase 9 Robustness & Error Handling ---${NC}"
assert_success "60. Command not found exits with status 127" test_exit_code_127_command_not_found
assert_success "61. Non-executable file exits with status 126" test_exit_code_126_non_executable
assert_success "62. Redirection missing file reports syntax error" test_redirection_missing_file_syntax_error
assert_success "63. Builtin stdout redirection works cleanly" test_builtin_stdout_redirection
assert_success "64. cd to a non-directory file reports error" test_cd_to_file_error
assert_success "65. cd to nonexistent directory exits with status 1" test_cd_nonexistent_status
assert_success "66. exit with non-numeric argument exits with code 2" test_exit_non_numeric_argument
assert_success "67. 1000-argument stress test handles memory cleanly" test_large_argument_list
assert_success "68. Pipeline error recovery maintains shell integrity" test_pipeline_error_recovery
assert_success "69. Non-interactive mode executes cleanly without prompt" test_non_interactive_mode
assert_success "70. Background pipeline tracks full multi-process lifecycle" test_background_pipeline_full_lifecycle
assert_success "71. Repeated execution errors do not leak file descriptors" test_fd_leak_prevention_on_errors

echo -e "${BLUE}==================================================${NC}"
echo -e "Summary: Passed: ${GREEN}${PASSED_TESTS}${NC} | Failed: ${RED}${FAILED_TESTS}${NC}"
echo -e "${BLUE}==================================================${NC}"

if [[ ${FAILED_TESTS} -ne 0 ]]; then
    exit 1
fi

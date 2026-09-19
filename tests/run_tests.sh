#!/usr/bin/env bash
# Test harness for Mini Unix Shell - Phase 4

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASSED_TESTS=0
FAILED_TESTS=0

echo -e "${BLUE}==================================================${NC}"
echo -e "${BLUE}  Mini Unix Shell - Test Suite (Phase 4)         ${NC}"
echo -e "${BLUE}==================================================${NC}"

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
    [[ "$v1" == *"mini-shell version 0.5.0-phase5"* ]] && \
[[ "$v2" == *"mini-shell version 0.5.0-phase5"* ]]
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

test_eof_termination() { printf "" | ./bin/mini-shell; }

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
    output=$(printf "cd\npwd\n" | ./bin/mini-shell)
    [[ "$output" == "$HOME" ]]
}

test_cd_nonexistent_directory() {
    local output
    output=$(printf "cd /nonexistent_directory_xyz\nexit\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: cd:"* ]]
}

test_cd_too_many_arguments() {
    local output
    output=$(printf "cd /tmp /\nexit\n" | ./bin/mini-shell 2>&1)
    [[ "$output" == *"mini-shell: cd: too many arguments"* ]]
}

test_help_builtin() {
    output=$(printf "help\n" | ./bin/mini-shell)
    [[ "$output" == *"cd [dir]"* ]] && \
    [[ "$output" == *"pwd"* ]] && \
    [[ "$output" == *"exit [n]"* ]] && \
    [[ "$output" == *"help"* ]]
}

test_exit_zero() {
    local status=0
    printf "exit\n" | ./bin/mini-shell >/dev/null || status=$?
    [[ $status -eq 0 ]]
}

test_exit_custom_status() {
    local status=0
    printf "exit 42\n" | ./bin/mini-shell >/dev/null || status=$?
    [[ $status -eq 42 ]]
}

test_exit_too_many_arguments() {
    local output
    output=$(printf "exit 1 2\necho still-running\nexit\n" | ./bin/mini-shell 2>&1)
    [[ "$output" == *"mini-shell: exit: too many arguments"* ]] && \
    [[ "$output" == *"still-running"* ]]
}

test_cd_persists_for_external_command() {
    output=$(printf "cd $PROJECT_ROOT/docs\n/bin/pwd\n" | ./bin/mini-shell)
    [[ "$output" == "$PROJECT_ROOT/docs" ]]
}

# --- Phase 3 Redirection Tests ---

test_output_redirection() {
    local file="${PROJECT_ROOT}/test-output.txt"
    rm -f "$file"

    printf "echo hello > %s\n" "$file" | ./bin/mini-shell >/dev/null

    [[ -f "$file" ]] && [[ "$(cat "$file")" == "hello" ]]

    rm -f "$file"
}

test_output_redirection_truncates() {
    local file="${PROJECT_ROOT}/test-output.txt"
    rm -f "$file"

    printf "echo first > %s\necho second > %s\n" "$file" "$file" |
        ./bin/mini-shell >/dev/null

    [[ "$(cat "$file")" == "second" ]]

    rm -f "$file"
}

test_append_redirection() {
    local file="${PROJECT_ROOT}/test-output.txt"
    rm -f "$file"

    printf "echo first > %s\necho second >> %s\n" "$file" "$file" |
        ./bin/mini-shell >/dev/null

    expected=$(printf "first\nsecond")
    [[ "$(cat "$file")" == "$expected" ]]

    rm -f "$file"
}

test_input_redirection() {
    local input="${PROJECT_ROOT}/test-input.txt"
    rm -f "$input"

    printf "hello from input\n" > "$input"

    output=$(printf "cat < %s\n" "$input" | ./bin/mini-shell)

    [[ "$output" == "hello from input" ]]

    rm -f "$input"
}

test_missing_input_file() {
    local missing="${PROJECT_ROOT}/does-not-exist.txt"
    rm -f "$missing"

    local output
    output=$(printf "cat < %s\n" "$missing" | ./bin/mini-shell 2>&1 || true)

    [[ "$output" == *"mini-shell: input redirection:"* ]]
}

test_redirection_does_not_pass_operator_to_command() {
    local file="${PROJECT_ROOT}/test-output.txt"
    rm -f "$file"

    printf "echo hello > %s\n" "$file" | ./bin/mini-shell >/dev/null

    [[ "$(cat "$file")" == "hello" ]]

    rm -f "$file"
}

# --- Phase 4 Single Pipe Tests ---

test_basic_pipe() {
    output=$(printf "echo hello | cat\n" | ./bin/mini-shell)
    [[ "$output" == "hello" ]]
}

test_pipe_line_count() {
    output=$(printf "echo hello | wc -l\n" | ./bin/mini-shell)
    [[ "$output" =~ ^[[:space:]]*1[[:space:]]*$ ]]
}

test_pipe_word_count() {
    output=$(printf "echo hello world | wc -w\n" | ./bin/mini-shell)
    [[ "$output" =~ ^[[:space:]]*2[[:space:]]*$ ]]
}

test_pipe_preserves_arguments() {
    output=$(printf "echo hello world | cat\n" | ./bin/mini-shell)
    [[ "$output" == "hello world" ]]
}

test_pipe_first_command_failure() {
    local output
    output=$(printf "abcxyz | cat\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: abcxyz: No such file or directory"* ]]
}

test_pipe_second_command_failure() {
    local output
    output=$(printf "echo hello | abcxyz\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: abcxyz: No such file or directory"* ]]
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

# --- Phase 5 Multiple Pipelines Tests ---

test_three_command_pipeline() {
    output=$(printf "echo apple | grep apple | wc -l\n" | ./bin/mini-shell)
    [[ "$output" =~ ^[[:space:]]*1[[:space:]]*$ ]]
}

test_four_command_pipeline() {
    output=$(printf "echo hello | cat | cat | wc -w\n" | ./bin/mini-shell)
    [[ "$output" =~ ^[[:space:]]*1[[:space:]]*$ ]]
}

test_multiple_pipeline_arguments() {
    output=$(printf "echo hello world | wc -w | cat\n" | ./bin/mini-shell)
    [[ "$output" =~ ^[[:space:]]*2[[:space:]]*$ ]]
}

test_multiple_pipeline_invalid_middle() {
    local output
    output=$(printf "echo hello || wc\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: invalid pipe"* ]]
}

test_multiple_pipeline_missing_command() {
    local output
    output=$(printf "echo hello | | wc\n" | ./bin/mini-shell 2>&1 || true)
    [[ "$output" == *"mini-shell: invalid pipe"* ]]
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
assert_success "21. help lists all four built-ins" test_help_builtin
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

echo -e "${BLUE}==================================================${NC}"
echo -e "Summary: Passed: ${GREEN}${PASSED_TESTS}${NC} | Failed: ${RED}${FAILED_TESTS}${NC}"
echo -e "${BLUE}==================================================${NC}"

if [[ ${FAILED_TESTS} -ne 0 ]]; then
    exit 1
fi

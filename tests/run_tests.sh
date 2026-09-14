#!/usr/bin/env bash
# Test harness for Mini Unix Shell - Phase 1

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
echo -e "${BLUE}  Mini Unix Shell - Test Suite (Phase 1)          ${NC}"
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

test_binary_exists() {
    [[ -x "./bin/mini-shell" ]]
}

test_version_flag() {
    v1=$(./bin/mini-shell --version)
    v2=$(./bin/mini-shell -v)
    [[ "$v1" == *"mini-shell version 0.1.0-phase1"* ]] && \
    [[ "$v2" == *"mini-shell version 0.1.0-phase1"* ]]
}

test_help_flag() {
    h1=$(./bin/mini-shell --help)
    h2=$(./bin/mini-shell -h)
    [[ "$h1" == *"Usage:"* ]] && [[ "$h2" == *"Usage:"* ]]
}

test_invalid_flag() {
    ./bin/mini-shell --invalid-flag
}

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
    # Empty input should terminate cleanly with code 0
    printf "" | ./bin/mini-shell
}

test_nonexistent_command_error() {
    local err_output
    err_output=$(echo "nonexistent_command_12345" | ./bin/mini-shell 2>&1 || true)
    [[ "$err_output" == *"mini-shell: nonexistent_command_12345"* ]]
}

test_command_failure_exit_status() {
    # Child failure status should propagate to shell exit code
    echo "ls /path/that/does/not/exist_12345" | ./bin/mini-shell
}

# Execute tests
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

echo -e "${BLUE}==================================================${NC}"
echo -e "Summary: Passed: ${GREEN}${PASSED_TESTS}${NC} | Failed: ${RED}${FAILED_TESTS}${NC}"
echo -e "${BLUE}==================================================${NC}"

if [[ ${FAILED_TESTS} -ne 0 ]]; then
    exit 1
fi

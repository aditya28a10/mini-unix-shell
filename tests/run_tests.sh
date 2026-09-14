#!/usr/bin/env bash
# Test harness for Mini Unix Shell - Phase 0

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
echo -e "${BLUE}  Mini Unix Shell - Phase 0 Test Suite            ${NC}"
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

# Test 1: Executable exists and is executable
test_binary_exists() {
    [[ -x "./bin/mini-shell" ]]
}

# Test 2: Default invocation runs cleanly
test_default_run() {
    output=$(./bin/mini-shell)
    echo "$output" | grep -q "mini-shell (0.0.1-phase0)" && \
    echo "$output" | grep -q "Phase 0 environment initialized successfully"
}

# Test 3: Version flag (--version and -v)
test_version_flag() {
    v1=$(./bin/mini-shell --version)
    v2=$(./bin/mini-shell -v)
    [[ "$v1" == *"mini-shell version 0.0.1-phase0"* ]] && \
    [[ "$v2" == *"mini-shell version 0.0.1-phase0"* ]]
}

# Test 4: Help flag (--help and -h)
test_help_flag() {
    h1=$(./bin/mini-shell --help)
    h2=$(./bin/mini-shell -h)
    [[ "$h1" == *"Usage:"* ]] && [[ "$h2" == *"Usage:"* ]]
}

# Test 5: Invalid flag returns non-zero status
test_invalid_arg() {
    ./bin/mini-shell --invalid-flag
}

# Test 6: Clean target removes artifacts
test_make_clean() {
    make clean >/dev/null
    [[ ! -d "./bin" ]] && [[ ! -d "./obj" ]]
}

# Test 7: Rebuilds cleanly after clean
test_rebuild() {
    make all >/dev/null
    [[ -x "./bin/mini-shell" ]]
}

# Execute tests
assert_success "1. Binary exists and is executable" test_binary_exists
assert_success "2. Default invocation prints banner and exits 0" test_default_run
assert_success "3. Version flags (--version, -v) work" test_version_flag
assert_success "4. Help flags (--help, -h) work" test_help_flag
assert_failure "5. Unrecognized flag exits with non-zero code" test_invalid_arg
assert_success "6. 'make clean' removes bin/ and obj/ directories" test_make_clean
assert_success "7. 'make all' rebuilds binary cleanly" test_rebuild

echo -e "${BLUE}==================================================${NC}"
echo -e "Summary: Passed: ${GREEN}${PASSED_TESTS}${NC} | Failed: ${RED}${FAILED_TESTS}${NC}"
echo -e "${BLUE}==================================================${NC}"

if [[ ${FAILED_TESTS} -ne 0 ]]; then
    exit 1
fi

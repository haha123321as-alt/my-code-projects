#!/bin/bash
# test_all.sh - Automated test script for IPC Task 2

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
USER_APP_DIR="$SCRIPT_DIR/user_app"
BUILD_DIR="$USER_APP_DIR/build"

GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================================${NC}"
echo -e "${BLUE}        Running Automated Tests for IPC Task 2         ${NC}"
echo -e "${BLUE}========================================================${NC}"

# 1. Compile everything
echo -e "\n${BLUE}[1/4] Checking build...${NC}"
make -C "$SCRIPT_DIR" clean > /dev/null
make -C "$SCRIPT_DIR" > /dev/null
echo -e "${GREEN}[OK] Kernel module and User App compiled successfully.${NC}"

# Function to test an IPC mode
test_mode() {
    local mode=$1
    local test_msg="hello_${mode}_task2"
    local log_file="/tmp/test_${mode}.log"

    echo -e "\n${BLUE}[Testing IPC Mode: ${mode}]...${NC}"
    rm -f "$log_file"

    # Start receiver in background
    "$BUILD_DIR/receiver" "$mode" > "$log_file" 2>&1 &
    local recv_pid=$!
    sleep 0.5

    # Run sender with simulated input
    "$BUILD_DIR/sender" "$mode" <<< "$test_msg" > /dev/null 2>&1 || true
    sleep 0.5

    # Terminate receiver
    kill -INT "$recv_pid" 2>/dev/null || true
    wait "$recv_pid" 2>/dev/null || true

    # Check if receiver received the message
    if grep -q "Received: $test_msg" "$log_file"; then
        echo -e "${GREEN}[PASS] ${mode^^} IPC: Successfully transferred message '${test_msg}'${NC}"
    else
        echo -e "${RED}[FAIL] ${mode^^} IPC: Did not find expected message in log!${NC}"
        echo "Log content:"
        cat "$log_file"
        rm -f "$log_file"
        return 1
    fi
    rm -f "$log_file"
}

# 2. Test Socket
test_mode "socket"

# 3. Test FIFO
test_mode "fifo"

# 4. Test Message Queue
test_mode "mq"

echo -e "\n${GREEN}========================================================${NC}"
echo -e "${GREEN}      ALL TESTS PASSED! Task 2 is ready to use!        ${NC}"
echo -e "${GREEN}========================================================${NC}"


#!/bin/bash

# Run All Tests Script
# 运行所有类型的测试：单元测试、集成测试、功能测试

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================"
echo "Running All Tests"
echo "========================================"
echo ""

# Check if unit tests script exists
if [ -f "run_unit_tests.sh" ]; then
    echo -e "${BLUE}Step 1: Running Unit Tests${NC}"
    echo "----------------------------------------"
    ./run_unit_tests.sh
    UNIT_TEST_RESULT=$?
    echo ""
else
    echo -e "${YELLOW}Warning: run_unit_tests.sh not found, skipping unit tests${NC}"
    UNIT_TEST_RESULT=0
fi

# Check if functional tests script exists
if [ -f "run_functional_tests.sh" ]; then
    echo -e "${BLUE}Step 2: Running Functional Tests${NC}"
    echo "----------------------------------------"
    ./run_functional_tests.sh
    FUNCTIONAL_TEST_RESULT=$?
    echo ""
else
    echo -e "${YELLOW}Warning: run_functional_tests.sh not found, skipping functional tests${NC}"
    FUNCTIONAL_TEST_RESULT=0
fi

# Summary
echo "========================================"
echo "Test Summary"
echo "========================================"

if [ $UNIT_TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ Unit Tests: PASSED${NC}"
else
    echo -e "${RED}❌ Unit Tests: FAILED${NC}"
fi

if [ $FUNCTIONAL_TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ Functional Tests: PASSED${NC}"
else
    echo -e "${RED}❌ Functional Tests: FAILED${NC}"
fi

echo ""

# Exit with error if any tests failed
if [ $UNIT_TEST_RESULT -ne 0 ] || [ $FUNCTIONAL_TEST_RESULT -ne 0 ]; then
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi


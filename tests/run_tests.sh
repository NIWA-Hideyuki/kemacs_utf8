#!/bin/sh
# run_tests.sh - Batch test harness for kemacs.
# Builds and runs all unit tests in the tests/ directory.
# Exits non-zero if any test fails.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building unit tests ==="
make all

echo ""
echo "=== Running unit tests ==="
./test_kanji
./test_search
./test_line
./test_eval

echo ""
echo "=== Running batch tests ==="
./test_batch

echo ""
echo "=== All tests passed ==="

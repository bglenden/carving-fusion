#!/bin/bash
# Run clang-tidy on all source files
# Usage: run_clang_tidy.sh <clang-tidy-path> <build-dir>

CLANG_TIDY="$1"
BUILD_DIR="$2"

if [ -z "$CLANG_TIDY" ] || [ -z "$BUILD_DIR" ]; then
    echo "Usage: $0 <clang-tidy-path> <build-dir>"
    exit 1
fi

# Find all .cpp files in src/ and run clang-tidy
find src -name "*.cpp" -print0 | xargs -0 "$CLANG_TIDY" -p "$BUILD_DIR" 2>&1

exit ${PIPESTATUS[1]}

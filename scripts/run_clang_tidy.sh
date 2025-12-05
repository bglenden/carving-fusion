#!/bin/bash
# Run clang-tidy on all source files using project's .clang-tidy config
# This matches exactly what Zed/clangd sees
# Usage: run_clang_tidy.sh <clang-tidy-path> <build-dir>

CLANG_TIDY="$1"
BUILD_DIR="$2"

if [ -z "$CLANG_TIDY" ] || [ -z "$BUILD_DIR" ]; then
    echo "Usage: $0 <clang-tidy-path> <build-dir>"
    exit 1
fi

# Create temp files
TEMP_RAW=$(mktemp)
TEMP_FILTERED=$(mktemp)
trap "rm -f $TEMP_RAW $TEMP_FILTERED" EXIT

echo "Running clang-tidy on source files (using .clang-tidy config)..."
echo ""

# Find all .cpp files in src/ and run clang-tidy
# Uses the project's .clang-tidy file automatically (same as Zed/clangd)
find src -name "*.cpp" -print0 | xargs -0 "$CLANG_TIDY" \
    -p "$BUILD_DIR" \
    2>&1 > "$TEMP_RAW"

# Filter to only show warnings from our code (carving-fusion/src or carving-fusion/include)
# Exclude Fusion API headers, system headers, boost, and openvoronoi
grep -E "carving-fusion/(src|include)/.*warning:" "$TEMP_RAW" | \
    grep -v "/Autodesk/" | \
    grep -v "/openvoronoi/" | \
    grep -v "/boost/" | \
    grep -v "system header" > "$TEMP_FILTERED"

# Show filtered warnings
if [ -s "$TEMP_FILTERED" ]; then
    echo "Warnings in project code:"
    echo "-------------------------"
    cat "$TEMP_FILTERED"
fi

# Count and summarize
OUR_WARNINGS=$(wc -l < "$TEMP_FILTERED" | tr -d ' ')

echo ""
echo "=========================================="
echo "clang-tidy summary: $OUR_WARNINGS warning(s) in project code"
echo "=========================================="

# Show warning type breakdown if there are warnings
if [ "$OUR_WARNINGS" -gt 0 ]; then
    echo ""
    echo "Warning types:"
    grep -oE '\[[a-zA-Z0-9,-]+\]$' "$TEMP_FILTERED" | sort | uniq -c | sort -rn
fi

exit 0

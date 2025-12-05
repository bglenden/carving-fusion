#!/bin/bash
# Run clang-tidy using clangd LSP (same as Zed editor)
# This uses the .clangd config file and is much faster than standalone clang-tidy
# Usage: run_clang_tidy.sh <clang-tidy-path> <build-dir>

# Note: clang-tidy-path and build-dir args are kept for CMake compatibility but not used
# We use clangd instead which reads .clangd config

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

# Use the Python clangd lint script
python3 "$SCRIPT_DIR/clangd_lint.py"

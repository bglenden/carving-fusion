#!/bin/bash
# Coverage analysis script for C++ tests

set -e

echo "🔍 C++ Test Coverage Analysis"
echo "============================"

# Check for lcov
if ! command -v lcov &> /dev/null; then
    echo "⚠️  lcov not found. Installing with homebrew..."
    if command -v brew &> /dev/null; then
        brew install lcov
    else
        echo "❌ Please install lcov manually"
        exit 1
    fi
fi

# Clean previous build
echo "🧹 Cleaning previous coverage data..."
rm -rf build_coverage
mkdir -p build_coverage
cd build_coverage

# Configure with coverage enabled
echo "🔧 Configuring with coverage enabled..."
cmake .. -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -g -O0" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage"

# Build tests
echo "🏗️  Building tests with coverage instrumentation..."
make chip_carving_tests -j

# Run tests
echo "🧪 Running tests..."
./tests/chip_carving_tests

# Generate basic coverage summary using gcov
echo "📊 Generating coverage summary..."
cd tests/CMakeFiles/chip_carving_tests.dir

# Find all .gcda files and run gcov
total_lines=0
covered_lines=0

echo ""
echo "Coverage by module:"
echo "==================="

for gcda in $(find . -name "*.gcda" | grep -E "(geometry|parsers|core|commands)" | sort); do
    # Extract the source file path
    source_file=$(echo $gcda | sed 's/\.gcda$//')
    
    # Run gcov and parse output
    if [ -f "$gcda" ]; then
        gcov_output=$(gcov -n "$source_file" 2>/dev/null | grep -E "Lines executed:")
        if [ ! -z "$gcov_output" ]; then
            # Extract percentage and file name
            percentage=$(echo "$gcov_output" | grep -oE "[0-9]+\.[0-9]+%" | head -1)
            file_name=$(basename "$source_file" .cpp.gcda)
            printf "%-40s %s\n" "$file_name:" "$percentage"
        fi
    fi
done

echo ""
echo "==================="
echo "✅ Coverage analysis complete!"
echo ""
echo "Note: For detailed HTML reports, install lcov:"
echo "  brew install lcov"
echo "  Then re-run this script"
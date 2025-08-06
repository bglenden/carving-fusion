#!/bin/bash
# Analyze test coverage for non-Fusion dependent code
# This helps identify areas that need better test coverage without requiring fragile Fusion API mocks

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "Test Coverage Analysis for Non-Fusion Code"
echo "========================================="
echo ""

# Check if lcov is installed
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov is not installed${NC}"
    echo "Install with: brew install lcov"
    exit 1
fi

# Check if we're in the build directory
if [ ! -f "CMakeCache.txt" ]; then
    echo -e "${RED}Error: Not in build directory${NC}"
    echo "Please run from the build directory"
    exit 1
fi

# Clean previous coverage data
echo "Cleaning previous coverage data..."
find . -name "*.gcda" -delete 2>/dev/null || true
find . -name "*.gcno" -delete 2>/dev/null || true
rm -f coverage.info coverage_filtered.info 2>/dev/null || true

# Configure with coverage flags
echo "Configuring with coverage enabled..."
cmake .. -DENABLE_COVERAGE=ON \
         -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
         -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
         -DCMAKE_BUILD_TYPE=Debug > /dev/null 2>&1

# Build the tests
echo "Building tests with coverage instrumentation..."
make chip_carving_tests -j8 > /dev/null 2>&1

# Run the tests
echo "Running tests..."
./tests/chip_carving_tests --gtest_output=xml:test_results.xml > /dev/null 2>&1

# Capture coverage data
echo "Capturing coverage data..."
lcov --capture --directory . --output-file coverage.info --quiet --ignore-errors inconsistent,unsupported

# Filter out system headers and Fusion-dependent code
echo "Filtering coverage data (removing Fusion-dependent and system files)..."
lcov --remove coverage.info \
     '/usr/*' \
     '/opt/*' \
     '*/tests/*' \
     '*/build/*' \
     '*/Library/*' \
     '*/src/adapters/Fusion*' \
     '*/src/adapters/IFusionInterface.h' \
     '*/src/commands/HelloWorldCommand*' \
     '*/src/commands/SettingsCommand*' \
     '*/src/core/PluginInitializer*' \
     '*/src/main.cpp' \
     --output-file coverage_filtered.info --quiet --ignore-errors inconsistent,unsupported

# Generate summary
echo ""
echo "========================================="
echo "Coverage Summary for Testable Components"
echo "========================================="
lcov --list coverage_filtered.info | grep -E "^SF:|lines\.\.\." | while read -r line; do
    if [[ $line == SF:* ]]; then
        # Extract filename
        file=$(echo "$line" | sed 's/SF://')
        filename=$(basename "$file")
    elif [[ $line == *"lines..."* ]]; then
        # Extract coverage percentage
        coverage=$(echo "$line" | awk '{print $2}')
        percent=$(echo "$coverage" | sed 's/%//')
        
        # Color code based on coverage
        if (( $(echo "$percent < 50" | bc -l) )); then
            color=$RED
            status="POOR"
        elif (( $(echo "$percent < 80" | bc -l) )); then
            color=$YELLOW
            status="FAIR"
        else
            color=$GREEN
            status="GOOD"
        fi
        
        printf "${color}%-40s %6s%% %-6s${NC}\n" "$filename" "$coverage" "[$status]"
    fi
done

# Find files with poor coverage that are good candidates for testing
echo ""
echo "========================================="
echo "Priority Files for Additional Testing"
echo "========================================="
echo "(Non-Fusion files with < 80% coverage)"
echo ""

lcov --list coverage_filtered.info 2>/dev/null | grep -E "\.(cpp|h)" | while read -r line; do
    if [[ $line == *"%"* ]]; then
        file=$(echo "$line" | awk '{print $1}')
        coverage=$(echo "$line" | awk '{print $2}' | sed 's/%//')
        
        # Skip if coverage is good
        if (( $(echo "$coverage >= 80" | bc -l) )); then
            continue
        fi
        
        # Check if it's a geometry or parser file (good test candidates)
        if [[ $file == *"geometry"* ]] || [[ $file == *"parser"* ]] || [[ $file == *"utils"* ]]; then
            printf "  ${YELLOW}%-50s %6.1f%%${NC}\n" "$(basename $file)" "$coverage"
        fi
    fi
done

# Generate detailed HTML report
echo ""
echo "Generating HTML coverage report..."
genhtml coverage_filtered.info --output-directory coverage_report --quiet

echo ""
echo "========================================="
echo "Coverage Report Generated"
echo "========================================="
echo "HTML report: coverage_report/index.html"
echo "Open with: open coverage_report/index.html"
echo ""

# Show untested functions in key files
echo "========================================="
echo "Untested Functions in Core Components"
echo "========================================="
echo ""

# Look for completely untested functions in geometry classes
for file in ../src/geometry/*.cpp ../src/parsers/*.cpp ../src/utils/*.cpp; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")
        # Skip Fusion-dependent files
        if [[ $filename == *"Fusion"* ]]; then
            continue
        fi
        
        # Use lcov to find untested functions
        untested=$(lcov --list coverage_filtered.info 2>/dev/null | grep "$filename" -A 100 | grep "0.0%" | head -3)
        if [ ! -z "$untested" ]; then
            echo "In $filename:"
            echo "$untested" | while read -r line; do
                echo "  - $line"
            done
            echo ""
        fi
    fi
done

# Provide recommendations
echo "========================================="
echo "Testing Recommendations"
echo "========================================="
echo ""
echo "1. Focus on geometry classes (MedialAxisProcessor, Shape, etc.)"
echo "2. Test parsers (DesignParser) with edge cases"
echo "3. Add tests for utility functions"
echo "4. Avoid testing Fusion-dependent code directly"
echo "5. Use mock objects for adapter interfaces"
echo ""

# Calculate overall coverage
total_lines=$(lcov --summary coverage_filtered.info 2>&1 | grep "lines\.\.\." | awk '{print $2}')
echo "Overall coverage for non-Fusion code: ${GREEN}${total_lines}${NC}"
#!/bin/bash

# Coordinate System Regression Test Runner
# Runs critical tests to prevent coordinate system bugs that cause medial axis misalignment

set -e

echo "🔍 Running Coordinate System Regression Tests..."
echo "   These tests prevent bugs where medial axis appears offset from shape boundaries"
echo ""

# Navigate to test directory
cd "$(dirname "$0")/tests"

# Build tests if needed
if [ ! -f "chip_carving_tests" ] || [ "../src/adapters/FusionWorkspaceCurve.cpp" -nt "chip_carving_tests" ]; then
    echo "📦 Building tests..."
    make chip_carving_tests
    echo ""
fi

# Run the coordinate system regression tests
echo "🧪 Running coordinate system regression tests..."
./chip_carving_tests --gtest_filter="CoordinateSystemRegressionTest.*" --gtest_brief=1

# Check test result
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ All coordinate system regression tests PASSED"
    echo "   - Medial axis alignment is correct"
    echo "   - Scale consistency is maintained"
    echo "   - Origin independence is verified"
    echo "   - Boundary constraints are satisfied"
    echo ""
    echo "💡 These tests guard against coordinate system bugs that previously caused"
    echo "   medial axis to appear offset/rotated relative to shape boundaries."
else
    echo ""
    echo "❌ COORDINATE SYSTEM REGRESSION DETECTED!"
    echo ""
    echo "🚨 Critical Error: The medial axis coordinate system is broken."
    echo "   This likely means:"
    echo "   - extractProfileVertices() is returning local instead of world coordinates"
    echo "   - getCurveWorldGeometry() is using geometry() instead of worldGeometry()"
    echo "   - Coordinate transformations are being applied incorrectly"
    echo ""
    echo "📍 Check these files for coordinate system bugs:"
    echo "   - src/adapters/FusionWorkspaceCurve.cpp"
    echo "   - src/adapters/FusionWorkspaceProfile.cpp"
    echo "   - src/geometry/MedialAxisProcessorVoronoi.cpp"
    echo ""
    exit 1
fi
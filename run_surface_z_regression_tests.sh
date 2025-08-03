#!/bin/bash

# Surface Z Detection Regression Test Runner
# Runs critical tests to prevent surface Z detection bugs that cause V-carve misplacement

set -e

echo "🔍 Running Surface Z Detection Regression Tests..."
echo "   These tests prevent bugs where V-carve paths appear on wrong surface"
echo ""

# Navigate to test directory
cd "$(dirname "$0")/tests"

# Build tests if needed
if [ ! -f "chip_carving_tests" ] || [ "../src/adapters/FusionWorkspaceCurve.cpp" -nt "chip_carving_tests" ]; then
    echo "📦 Building tests..."
    make chip_carving_tests
    echo ""
fi

# Run the surface Z detection regression tests
echo "🧪 Running surface Z detection regression tests..."
./chip_carving_tests --gtest_filter="SurfaceZDetectionRegressionTest.*" --gtest_brief=1

# Check test result
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ All surface Z detection regression tests PASSED"
    echo "   - Surface Z range validation correct"
    echo "   - V-carve depth calculations accurate"
    echo "   - Coordinate system consistency maintained"
    echo "   - Top surface selection logic working"
    echo "   - Edge case handling robust"
    echo ""
    echo "💡 These tests guard against surface detection bugs that previously caused"
    echo "   V-carve paths to appear below bottom surface instead of top surface."
else
    echo ""
    echo "❌ SURFACE Z DETECTION REGRESSION DETECTED!"
    echo ""
    echo "🚨 Critical Error: The surface Z detection system is broken."
    echo "   This likely means:"
    echo "   - Ray casting is not selecting topmost surface"
    echo "   - Coordinate conversion (mm ↔ cm) is incorrect"
    echo "   - V-carve depth calculation has regressed"
    echo "   - Parameter space iteration reverted to closest point (wrong surface)"
    echo ""
    echo "📍 Check these files for surface detection bugs:"
    echo "   - src/adapters/FusionWorkspaceCurve.cpp (getSurfaceZAtXY method)"
    echo "   - src/core/PluginManagerPaths.cpp (surface query and V-carve calculation)"
    echo "   - Surface selection logic (ray casting implementation)"
    echo ""
    exit 1
fi
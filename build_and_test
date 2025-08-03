#!/bin/bash
# Build and Test script for Chip Carving C++ Plugin
# Comprehensive build pipeline with testing

set -e  # Exit on any error

echo "🏗️  Chip Carving C++ Plugin - Build & Test Pipeline"
echo "=================================================="

# Clean previous build
if [ -d "build" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf build
fi

echo "📁 Creating build directory..."
mkdir build
cd build

echo "🔧 Configuring CMake..."
cmake ..

echo "🧪 Building and running tests..."
make chip_carving_tests

echo "⚡ Executing unit tests..."
./tests/chip_carving_tests --gtest_output=xml:test_results.xml

echo ""
echo "✅ All tests passed! Building plugins..."
echo ""

echo "🏗️  Building debug version..."
make chip_carving_paths_cpp

echo "🏗️  Building refactored version..."
make chip_carving_paths_cpp_refactored

echo ""
echo "🎉 Build Complete!"
echo "=================="
echo ""
echo "Built artifacts:"
echo "  📦 chip_carving_paths_cpp.dylib (debug version)"
echo "  📦 chip_carving_paths_cpp_refactored.dylib (testable version)"
echo "  🧪 tests/chip_carving_tests (unit test binary)"
echo "  📊 test_results.xml (test report)"
echo ""
echo "🚀 Ready for Fusion 360 deployment!"
echo ""
echo "Next steps:"
echo "  1. Copy .dylib files to Fusion 360 AddIns directory"
echo "  2. Copy manifest.json to plugin directory"
echo "  3. Copy resources/ directory to plugin directory"
echo ""
echo "To run tests again:"
echo "  ./tests/chip_carving_tests"
echo ""
echo "To run specific tests:"
echo "  ./tests/chip_carving_tests --gtest_filter=PluginManagerTest.*"
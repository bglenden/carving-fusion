#!/bin/bash
# Test runner script for Chip Carving C++ Plugin
# Builds and runs all unit tests

set -e  # Exit on any error

echo "🧪 Chip Carving C++ Plugin - Test Runner"
echo "========================================"

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "📁 Creating build directory..."
    mkdir build
fi

cd build

echo "🔧 Configuring CMake..."
cmake ..

echo "🏗️  Building tests..."
make chip_carving_tests

echo "🧪 Running unit tests..."
./tests/chip_carving_tests

echo ""
echo "✅ All tests passed!"
echo "🚀 Ready to build plugin for Fusion 360"
echo ""
echo "To build the plugin:"
echo "  make chip_carving_paths_cpp"
echo ""
echo "To install to Fusion 360:"
echo "  make install-fusion"
echo ""
echo "To run specific tests:"
echo "  ./tests/chip_carving_tests --gtest_filter=PluginManagerTest.*"
echo ""
echo "To run tests with verbose output:"
echo "  ./tests/chip_carving_tests --gtest_verbose"
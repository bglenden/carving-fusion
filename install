#!/bin/bash
# Install script for Chip Carving C++ Fusion 360 Plugin
# Uses proper make targets for build and install

set -e

echo "Installing Chip Carving C++ Plugin..."

# Change to build directory
cd build

# Build the plugin (this will also increment version)
echo "Building plugin..."
make chip_carving_paths_cpp

# Copy files to Fusion 360 directory
echo "Installing to Fusion 360..."
ADDINS_DIR="/Users/brianglendenning/Library/Application Support/Autodesk/Autodesk Fusion 360/API/AddIns"
PLUGIN_DIR="$ADDINS_DIR/chip_carving_paths_cpp"

# Create plugin directory if it doesn't exist
mkdir -p "$PLUGIN_DIR"

# Copy plugin files
cp ../chip_carving_paths_cpp.dylib "$PLUGIN_DIR/"
cp ../chip_carving_paths_cpp.manifest "$PLUGIN_DIR/"

# Copy resources
cp -r ../resources "$PLUGIN_DIR/"

echo ""
echo "✅ Installation complete!"
echo "Plugin installed to: $PLUGIN_DIR"
echo "🔄 Restart Fusion 360 to load the updated plugin."
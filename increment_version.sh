#!/bin/bash
# Auto-increment version script for Chip Carving C++ Plugin
# Run this before each build to increment the patch version

VERSION_FILE="VERSION"
MANIFEST_FILE="chip_carving_paths_cpp.manifest"

# Check if VERSION file exists
if [ ! -f "$VERSION_FILE" ]; then
    echo "Error: $VERSION_FILE not found"
    exit 1
fi

# Read current version
current_version=$(cat "$VERSION_FILE")

# Parse version (format: major.minor.patch)
if [[ ! "$current_version" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
    echo "Error: Invalid version format in $VERSION_FILE: $current_version"
    exit 1
fi

major="${BASH_REMATCH[1]}"
minor="${BASH_REMATCH[2]}"
patch="${BASH_REMATCH[3]}"

# Increment patch version
new_patch=$((patch + 1))
new_version="${major}.${minor}.${new_patch}"

# Update VERSION file
echo "$new_version" > "$VERSION_FILE"

# Update manifest file if it exists
if [ -f "$MANIFEST_FILE" ]; then
    # Use sed to update the version in the manifest
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS sed requires -i '' for in-place editing and different regex syntax
        sed -i '' "s/\"version\": \"[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\"/\"version\": \"$new_version\"/" "$MANIFEST_FILE"
    else
        # GNU sed
        sed -i "s/\"version\": \"[0-9]\+\.[0-9]\+\.[0-9]\+\"/\"version\": \"$new_version\"/" "$MANIFEST_FILE"
    fi
    echo "Updated manifest version to $new_version"
fi

echo "Version incremented to $new_version"

# Regenerate src/version.h from template using CMake
if [ -f "cmake/GenerateVersion.cmake" ]; then
    echo "Regenerating src/version.h..."
    cmake -DSOURCE_DIR="$(pwd)" -P cmake/GenerateVersion.cmake
    if [ $? -eq 0 ]; then
        echo "Successfully updated src/version.h"
    else
        echo "Warning: Failed to regenerate src/version.h"
    fi
else
    echo "Warning: cmake/GenerateVersion.cmake not found"
    echo "Please run cmake from the build directory to update src/version.h"
fi
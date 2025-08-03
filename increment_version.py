#!/usr/bin/env python3
"""
Auto-increment version script for Chip Carving C++ Plugin
Run this before each build to increment the patch version
"""

import re
import os

VERSION_FILE = "VERSION"
MANIFEST_FILE = "chip_carving_paths_cpp.manifest"

def increment_version():
    # Read current version from VERSION file
    if not os.path.exists(VERSION_FILE):
        print(f"Error: {VERSION_FILE} not found")
        return False
    
    with open(VERSION_FILE, 'r') as f:
        current_version = f.read().strip()
    
    # Parse version (format: major.minor.patch)
    version_match = re.match(r'(\d+)\.(\d+)\.(\d+)', current_version)
    if not version_match:
        print(f"Error: Invalid version format in {VERSION_FILE}: {current_version}")
        return False
    
    major, minor, patch = map(int, version_match.groups())
    new_patch = patch + 1
    new_version = f"{major}.{minor}.{new_patch}"
    
    # Update VERSION file
    with open(VERSION_FILE, 'w') as f:
        f.write(new_version)
    
    # Update manifest file
    if os.path.exists(MANIFEST_FILE):
        with open(MANIFEST_FILE, 'r') as f:
            manifest_content = f.read()
        
        # Update version in manifest
        manifest_content = re.sub(
            r'"version": "\d+\.\d+\.\d+"',
            f'"version": "{new_version}"',
            manifest_content
        )
        
        with open(MANIFEST_FILE, 'w') as f:
            f.write(manifest_content)
        
        print(f"Updated manifest version to {new_version}")
    
    print(f"Version incremented to {new_version}")
    return True

if __name__ == "__main__":
    increment_version()
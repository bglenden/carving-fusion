#!/usr/bin/env python3
"""
Script to fix common, easy lint issues automatically
Focuses on:
1. Trailing whitespace
2. Missing newlines at end of files  
3. TODO format (add generic username)
4. Comment spacing (// namespace -> }  // namespace)
"""

import os
import re
import glob

def fix_file(file_path):
    """Fix common lint issues in a single file"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        
        # Fix 1: Remove trailing whitespace from lines
        lines = content.split('\n')
        lines = [line.rstrip() for line in lines]
        
        # Fix 2: Ensure file ends with exactly one newline
        # Remove any trailing empty lines first
        while lines and lines[-1] == '':
            lines.pop()
        
        # Add back exactly one newline at end
        content = '\n'.join(lines) + '\n'
        
        # Fix 3: Fix TODO format - add generic username if missing
        content = re.sub(r'// TODO:', '// TODO(dev):', content)
        
        # Fix 4: Fix comment spacing after namespace closing braces
        content = re.sub(r'} // namespace', '}  // namespace', content)
        
        # Fix 5: Fix comment spacing - ensure at least 2 spaces before end-of-line comments
        content = re.sub(r'([^ ]) // ([^/])', r'\1  // \2', content)
        
        if content != original_content:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Fixed: {file_path}")
            return True
        else:
            return False
            
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return False

def main():
    """Fix lint issues in all source files"""
    src_root = "/Users/brianglendenning/SoftwareProjects/CNC_Chip_Carving/fusion_plugin_cpp/src"
    
    # Find all .cpp and .h files
    patterns = ['**/*.cpp', '**/*.h']
    files_to_fix = []
    
    for pattern in patterns:
        files_to_fix.extend(glob.glob(os.path.join(src_root, pattern), recursive=True))
    
    print(f"Found {len(files_to_fix)} files to process")
    
    fixed_count = 0
    for file_path in sorted(files_to_fix):
        if fix_file(file_path):
            fixed_count += 1
    
    print(f"Fixed {fixed_count} files")

if __name__ == '__main__':
    main()
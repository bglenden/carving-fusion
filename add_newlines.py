

import os

def add_newline_if_missing(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    if not content.endswith('\n'):
        print(f"Adding newline to: {filepath}")
        with open(filepath, 'a', encoding='utf-8') as f:
            f.write('\n')
    else:
        print(f"Newline already present: {filepath}")

def process_directory(directory):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cpp', '.h')):
                filepath = os.path.join(root, file)
                add_newline_if_missing(filepath)

if __name__ == "__main__":
    project_root = "/Users/brianglendenning/SoftwareProjects/carving/carving-fusion"
    process_directory(os.path.join(project_root, "src"))
    process_directory(os.path.join(project_root, "include"))
    print("Newline addition process complete.")


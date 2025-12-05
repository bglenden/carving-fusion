#!/usr/bin/env python3
"""
Fast clang-tidy linting using clangd LSP server.
Uses the same configuration as Zed (.clangd file).
"""

import subprocess
import json
import sys
import os
import select
import time
import glob

def send_request(proc, method, params, msg_id):
    msg = json.dumps({"jsonrpc": "2.0", "id": msg_id, "method": method, "params": params})
    content = f"Content-Length: {len(msg)}\r\n\r\n{msg}"
    proc.stdin.write(content.encode())
    proc.stdin.flush()

def send_notification(proc, method, params):
    msg = json.dumps({"jsonrpc": "2.0", "method": method, "params": params})
    content = f"Content-Length: {len(msg)}\r\n\r\n{msg}"
    proc.stdin.write(content.encode())
    proc.stdin.flush()

def read_response(proc, timeout=0.1):
    if not select.select([proc.stdout], [], [], timeout)[0]:
        return None
    headers = {}
    while True:
        line = proc.stdout.readline().decode().strip()
        if not line:
            break
        if ": " in line:
            key, value = line.split(": ", 1)
            headers[key] = value
    length = int(headers.get("Content-Length", 0))
    if length:
        body = proc.stdout.read(length).decode()
        return json.loads(body)
    return None

def lint_file(filepath, root_dir):
    """Lint a single file using clangd."""

    proc = subprocess.Popen(
        ["clangd", "--enable-config", "--clang-tidy"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=root_dir
    )

    diagnostics = []
    abs_path = os.path.abspath(os.path.join(root_dir, filepath))

    # Initialize
    send_request(proc, "initialize", {
        "processId": os.getpid(),
        "rootUri": f"file://{root_dir}",
        "capabilities": {}
    }, 1)

    start = time.time()
    initialized = False
    file_opened = False

    while time.time() - start < 5:
        resp = read_response(proc, 0.1)

        if resp:
            if resp.get("method") == "textDocument/publishDiagnostics":
                diagnostics.extend(resp.get("params", {}).get("diagnostics", []))
            elif "result" in resp and not initialized:
                initialized = True
                send_notification(proc, "initialized", {})

        if initialized and not file_opened:
            file_opened = True
            with open(abs_path) as f:
                text = f.read()
            send_notification(proc, "textDocument/didOpen", {
                "textDocument": {
                    "uri": f"file://{abs_path}",
                    "languageId": "cpp",
                    "version": 1,
                    "text": text
                }
            })

        if file_opened and diagnostics:
            time.sleep(0.2)
            while (resp := read_response(proc, 0.1)):
                if resp.get("method") == "textDocument/publishDiagnostics":
                    diagnostics.extend(resp.get("params", {}).get("diagnostics", []))
            break

    proc.terminate()
    proc.wait()
    return diagnostics

def main():
    root_dir = os.getcwd()

    # Get files from command line or find all
    if len(sys.argv) > 1:
        files = sys.argv[1:]
    else:
        files = glob.glob("src/**/*.cpp", recursive=True)

    if not files:
        print("No files to check")
        return 0

    print(f"Checking {len(files)} file(s) with clangd...")

    total_warnings = 0
    for filepath in files:
        diags = lint_file(filepath, root_dir)
        for d in diags:
            if d.get("severity", 1) <= 2:  # Error or Warning
                line = d.get("range", {}).get("start", {}).get("line", 0) + 1
                col = d.get("range", {}).get("start", {}).get("character", 0) + 1
                msg = d.get("message", "")
                code = d.get("code", "")
                print(f"{filepath}:{line}:{col}: warning: {msg} [{code}]")
                total_warnings += 1

    print(f"\n{'='*40}")
    print(f"clangd lint: {total_warnings} warning(s)")
    print(f"{'='*40}")

    return 0 if total_warnings == 0 else 1

if __name__ == "__main__":
    sys.exit(main())

#!/bin/bash
# Install git hooks for carving-fusion
# Run from repository root: ./hooks/install.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GIT_HOOKS_DIR="$REPO_ROOT/.git/hooks"

echo "Installing git hooks..."

# Handle worktrees (where .git is a file pointing to the real git dir)
if [ -f "$REPO_ROOT/.git" ]; then
    GIT_DIR=$(cat "$REPO_ROOT/.git" | sed 's/gitdir: //')
    GIT_HOOKS_DIR="$GIT_DIR/hooks"
fi

# Install pre-commit hook
if [ -f "$GIT_HOOKS_DIR/pre-commit" ]; then
    echo "Backing up existing pre-commit hook..."
    mv "$GIT_HOOKS_DIR/pre-commit" "$GIT_HOOKS_DIR/pre-commit.backup"
fi

cp "$SCRIPT_DIR/pre-commit" "$GIT_HOOKS_DIR/pre-commit"
chmod +x "$GIT_HOOKS_DIR/pre-commit"

echo "Installed pre-commit hook to $GIT_HOOKS_DIR/pre-commit"
echo "Done."

#!/bin/bash
# Check that source files don't exceed 350 lines as per project guidelines

MAX_LINES=350
VERBOSE=0

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --verbose) VERBOSE=1 ;;
        --max-lines) MAX_LINES="$2"; shift ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

# Store violations in a temp file to avoid subshell issues
TEMP_FILE=$(mktemp)
echo "0" > "$TEMP_FILE"

# Find all source files (script will be run from different directories)
find src include -type f \( -name "*.cpp" -o -name "*.h" \) 2>/dev/null | while read -r file; do
    lines=$(wc -l < "$file")
    if [ "$lines" -gt "$MAX_LINES" ]; then
        echo "ERROR: $file has $lines lines (exceeds $MAX_LINES line limit)"
        current=$(cat "$TEMP_FILE")
        echo $((current + 1)) > "$TEMP_FILE"
    elif [ "$VERBOSE" -eq 1 ] && [ "$lines" -gt $((MAX_LINES - 50)) ]; then
        echo "WARNING: $file has $lines lines (approaching $MAX_LINES line limit)"
    fi
done

VIOLATIONS=$(cat "$TEMP_FILE")
rm "$TEMP_FILE"

if [ "$VIOLATIONS" -gt 0 ]; then
    echo ""
    echo "Found $VIOLATIONS files exceeding the $MAX_LINES line limit"
    echo "Please refactor these files according to project guidelines"
    exit 1
fi

echo "All files are within the $MAX_LINES line limit"
exit 0
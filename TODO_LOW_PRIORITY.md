# Low Priority TODOs

**Purpose:** This file documents low-priority improvements identified during code review. These are not bugs - the current code works correctly for typical use cases. These are defensive improvements that would make the code more robust in edge cases.

**Standalone:** This file is designed to be usable in isolation. All context needed to implement these items is included here. You do not need to read other analysis or plan files.

---

## Issue 5: Profile Caching Without Invalidation Check

**Priority:** Low
**Location:** `src/adapters/FusionWorkspaceProfile.cpp`

### Problem

When extracting profile geometry, the code caches profile data but doesn't verify that the underlying Fusion 360 sketch hasn't been modified since caching. If a user:

1. Opens the Generate Paths dialog and selects profiles
2. Edits the sketch while the dialog is still open (adds/deletes/moves curves)
3. Clicks Execute

...the cached profile vertices would not match the current sketch geometry, causing toolpaths that don't match the visible shape.

### Why It's Low Priority

The typical workflow is: select profiles → click Execute immediately. Users rarely edit sketches with the plugin dialog open. This would only matter if users report "toolpaths don't match my sketch after I edited it."

### Current Code Pattern

```cpp
// Profile geometry is extracted and stored without tracking sketch revision
std::vector<ProfileInfo> cachedProfiles_;

ProfileInfo getProfile(size_t index) {
    if (index < cachedProfiles_.size()) {
        return cachedProfiles_[index];  // Returns cached without validation
    }
    // ...
}
```

### Recommended Fix

Use the Fusion 360 API's `Sketch::revisionId()` to detect when the sketch has changed:

```cpp
std::vector<ProfileInfo> cachedProfiles_;
std::string cachedRevisionId_;

ProfileInfo getProfile(size_t index) {
    // Check if sketch has been modified since caching
    auto currentRevision = sketch_->revisionId();
    if (currentRevision != cachedRevisionId_) {
        invalidateCache();
        cachedRevisionId_ = currentRevision;
    }

    if (index < cachedProfiles_.size()) {
        return cachedProfiles_[index];
    }
    // ... re-extract if needed
}
```

### Implementation Notes

- The `revisionId()` method returns a string that changes whenever the sketch is modified
- You'll need to store the revision ID when the cache is populated
- When the revision changes, clear the cache and re-extract geometry
- This affects the `GeneratePathsCommandHandler` workflow in `src/commands/PluginCommands*.cpp`

---

## Issue 8: size_t vs int in Collection Iteration

**Priority:** Low
**Location:** Multiple files throughout `src/adapters/` and `src/commands/`

### Problem

Fusion 360 API collections use `int` for `count()` and `item(int)`, but C++ best practices use `size_t` for sizes and indices. The current code handles this correctly with explicit casts, but it creates visual noise:

```cpp
// Current pattern (correct but noisy)
for (size_t i = 0; i < static_cast<size_t>(profiles->count()); i++) {
    auto profile = profiles->item(static_cast<int>(i));
    // ...
}
```

### Why It's Low Priority

The current code is correct - it just looks cluttered. This is purely a code cleanliness improvement with no functional impact.

### Recommended Fix

Create a template helper for cleaner iteration:

```cpp
// In a new file: src/utils/FusionCollectionIterator.h

#pragma once

#include <vector>

namespace ChipCarving {
namespace Utils {

/**
 * Convert a Fusion 360 collection to a std::vector for easier iteration.
 * Handles the int/size_t mismatch and provides range-based for loop support.
 *
 * Usage:
 *   for (auto& profile : toVector(sketch->profiles())) {
 *       // process profile
 *   }
 */
template<typename Collection, typename Item = decltype(std::declval<Collection>()->item(0))>
std::vector<Item> toVector(const adsk::core::Ptr<Collection>& collection) {
    std::vector<Item> items;
    if (!collection) return items;

    items.reserve(static_cast<size_t>(collection->count()));
    for (int i = 0; i < collection->count(); i++) {
        auto item = collection->item(i);
        if (item) {
            items.push_back(item);
        }
    }
    return items;
}

}  // namespace Utils
}  // namespace ChipCarving
```

### Files That Would Benefit

Search for `->count()` in these directories to find iteration patterns to update:
- `src/adapters/FusionWorkspace*.cpp`
- `src/adapters/FusionSketch*.cpp`
- `src/commands/PluginCommands*.cpp`

### Implementation Notes

- The helper creates a copy of the collection items, which has a small memory overhead
- For very large collections, the current inline approach may be preferred
- Add `isValid()` checks when extracting items (already done in most places per Issue 3)
- Consider adding a `toValidVector()` variant that filters out invalid items automatically

---

## Implementation Checklist

When implementing either issue:

1. Read CLAUDE.md for project conventions and commit requirements
2. Run `make test` before and after changes (all 287 tests must pass)
3. Run `make lint` (must show `Total errors found: 0`)
4. Run `make install` to verify the plugin builds
5. Commit with descriptive message following project conventions

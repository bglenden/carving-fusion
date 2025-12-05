# Fusion API Usage Review - carving-fusion

This document contains findings from a systematic review of the carving-fusion codebase against the official Autodesk Fusion 360 API documentation.

## Executive Summary

The codebase demonstrates **generally good API usage patterns** with a well-designed abstraction layer. However, there are several areas where improvements could enhance reliability, maintainability, and correctness.

**Overall Assessment:** Good foundation with room for improvement in specific areas.

---

## Strengths

### 1. Clean Abstraction Layer

- IFusionInterface.h defines clean interfaces (ILogger, IUserInterface, ISketch, IWorkspace, IFusionFactory)
- Enables dependency injection and testability
- Separates Fusion API concerns from business logic

### 2. Correct Use of findEntityByToken()

- FusionWorkspaceEntityLookup.cpp correctly uses Design.findEntityByToken() for O(1) entity lookup
- Good documentation explaining why this replaces manual iteration
- Proper fallback handling when lookup fails

### 3. Validity Checking Before deleteMe()

- FusionSketchConstruction.cpp:114-137 and FusionSketchCore.cpp:202-205 correctly check isValid() before calling deleteMe()
- Comments explain the pattern: Objects can become invalid after undo operations or model changes

### 4. Centralized Error Handling

- ErrorHandler.h/cpp provides consistent exception handling
- logApiError() in FusionWorkspace uses app->getLastError() for detailed diagnostics

### 5. Unit Conversion Centralized

- UnitConversion.h provides clear mm-to-cm conversion functions
- Consistent usage throughout the codebase

---

## Issues Found

### Issue 1: Event Handler Memory Management (MEDIUM PRIORITY) - ✅ IMPLEMENTED

**Location:** `src/commands/PluginCommandsCreation.cpp`, `src/commands/PluginCommandsImport.cpp`, `src/commands/SettingsCommand.cpp`

**Problem:** Event handlers were allocated with `new` but never explicitly deleted.

**Solution Implemented (Dec 2025):**

- Added `std::vector<CommandEventHandler*> commandEventHandlers_` to track allocated handlers
- Added `cleanupEventHandlers()` method to each command handler class
- Destructors now call `cleanupEventHandlers()` to delete all tracked handlers
- Applied Rule of 5 (deleted copy/move operations) to prevent handler pointer issues

---

### Issue 2: Inconsistent Unit Conversion in 3D Sketches (LOW PRIORITY)

**Location:** `src/adapters/FusionSketch3D.cpp:85-90`

**Problem:** Uses hardcoded `/10.0` division instead of the centralized `Utils::mmToFusionLength()` function.

**Current Code:**

```cpp
// FusionSketch3D.cpp
double x_cm = point.x / 10.0;
double y_cm = point.y / 10.0;
double z_cm = point.z / 10.0;
```

**Recommended Fix:**

```cpp
// Use centralized conversion
double x_cm = Utils::mmToFusionLength(point.x);
double y_cm = Utils::mmToFusionLength(point.y);
double z_cm = Utils::mmToFusionLength(point.z);
```

**Why This Matters:** If the conversion factor ever needs adjustment (e.g., for different unit systems), having it centralized ensures consistency.

---

### Issue 3: Missing isValid() Checks in Collection Iterations (MEDIUM PRIORITY) - ✅ IMPLEMENTED

**Location:** Multiple files (see below)

**Problem:** When iterating over profiles, loops, or curves, individual items weren't checked for validity before use.

**Solution Implemented (Dec 2025):**

Added `isValid()` checks to collection iterations in the following files:

- `src/adapters/FusionWorkspaceProfileSearch.cpp` - occurrences, sketches, profiles
- `src/adapters/FusionWorkspaceCurveUtils.cpp` - sketches, profiles
- `src/adapters/FusionWorkspaceProfileGeometry.cpp` - loops, curves
- `src/adapters/FusionWorkspaceCurveExtraction.cpp` - loops, curves
- `src/commands/PluginCommandsValidation.cpp` - profiles, loops, curves

**Pattern Used:**

```cpp
for (size_t i = 0; i < profiles->count(); i++) {
    auto profile = profiles->item(i);
    if (!profile || !profile->isValid()) {
        LOG_DEBUG("Skipping invalid profile at index " << i);
        continue;
    }
    auto loops = profile->profileLoops();
    // ...
}
```

**Why This Matters:** Collection contents can become invalid due to model edits, undo operations, or other API calls. Defensive checking prevents crashes.

---

### Issue 4: ValueInput Usage (NO ACTION NEEDED)

**Location:** Throughout codebase

**Status:** The codebase correctly uses `ValueInput::createByReal()` for programmatic values. This is the correct pattern per the API documentation.

**Example of Correct Usage:**

```cpp
// From FusionSketchCore.cpp - this is CORRECT
auto valueInput = ValueInput::createByReal(Utils::mmToFusionLength(distance));
```

The API documentation notes that `createByString()` is for user-entered expressions that may include units or formulas, while `createByReal()` is for programmatic values in internal units (cm).

---

### Issue 5: Profile Caching Without Invalidation Check (LOW PRIORITY)

**Location:** `src/adapters/FusionWorkspaceProfile.cpp:30-45`

**Problem:** Profiles are cached but the cache doesn't verify the underlying sketch hasn't changed.

**Current Code:**

```cpp
std::vector<ProfileInfo> cachedProfiles_;

ProfileInfo getProfile(size_t index) {
    if (index < cachedProfiles_.size()) {
        return cachedProfiles_[index];  // Returns cached without validation
    }
    // ...
}
```

**Recommended Fix:**

```cpp
std::vector<ProfileInfo> cachedProfiles_;
std::string cachedRevisionId_;

ProfileInfo getProfile(size_t index) {
    // Check if sketch has been modified
    auto currentRevision = sketch_->revisionId();
    if (currentRevision != cachedRevisionId_) {
        invalidateCache();
        cachedRevisionId_ = currentRevision;
    }
    // ...
}
```

**Why This Matters:** If the sketch is modified between cache population and access, stale profile data could cause incorrect behavior.

---

### Issue 6: Hard-Coded Tolerance Values (LOW PRIORITY)

**Location:** Multiple files

**Problem:** Geometric tolerance value `0.001` appears in several places without a named constant.

**Current Code:**

```cpp
// FusionSketchCore.cpp:150
if (std::abs(distance) < 0.001) {
    return;  // Skip negligible moves
}

// FusionWorkspaceCurveSurface.cpp:85
if (distanceToSurface < 0.001) {
    // Point is on surface
}
```

**Recommended Fix:**

```cpp
// In a constants header:
namespace Constants {
    constexpr double GEOMETRIC_TOLERANCE = 0.001;  // cm, Fusion internal units
    constexpr double POINT_COINCIDENCE_TOLERANCE = 0.0001;
}

// Usage:
if (std::abs(distance) < Constants::GEOMETRIC_TOLERANCE) {
    return;
}
```

**Why This Matters:** Centralizing tolerances makes them easier to tune and ensures consistent behavior across the codebase.

---

### Issue 7: Missing Null Checks in API Call Chains (MEDIUM PRIORITY)

**Location:** `src/adapters/FusionWorkspaceCurveSurface.cpp:120-140`

**Problem:** Some API call chains don't check intermediate results for null.

**Current Code:**

```cpp
auto body = occurrence->bRepBodies()->item(0);
auto faces = body->faces();  // Crashes if body is null
for (size_t i = 0; i < faces->count(); i++) {
    // ...
}
```

**Recommended Fix:**

```cpp
auto bodies = occurrence->bRepBodies();
if (!bodies || bodies->count() == 0) {
    logger_->warn("No bodies found in occurrence");
    return {};
}
auto body = bodies->item(0);
if (!body) {
    logger_->warn("Failed to get body");
    return {};
}
auto faces = body->faces();
if (!faces) {
    logger_->warn("Failed to get faces from body");
    return {};
}
// Now safe to iterate
```

**Why This Matters:** The Fusion API can return null/empty collections in various edge cases. Defensive null checking prevents crashes and provides better diagnostics.

---

### Issue 8: size_t vs int in Collection Iteration (LOW PRIORITY)

**Location:** Throughout codebase

**Problem:** Collections use `size_t` for count but Fusion API uses `int` internally. Current code handles this correctly but could be cleaner.

**Current Code:**

```cpp
for (size_t i = 0; i < static_cast<size_t>(profiles->count()); i++) {
    auto profile = profiles->item(static_cast<int>(i));
    // ...
}
```

**Recommended Fix:**

```cpp
// Create a helper for cleaner iteration
template<typename Collection>
auto iterateCollection(const Ptr<Collection>& collection) {
    std::vector<decltype(collection->item(0))> items;
    for (int i = 0; i < collection->count(); i++) {
        items.push_back(collection->item(i));
    }
    return items;
}

// Usage:
for (auto& profile : iterateCollection(profiles)) {
    // ...
}
```

**Why This Matters:** Reduces cast noise and makes iteration more idiomatic. Lower priority since current code is correct.

---

### Issue 9: Command Cleanup Order (LOW PRIORITY)

**Location:** `src/commands/PluginInitializer.cpp:80-95`

**Problem:** Command cleanup removes commands but doesn't handle potential failures gracefully.

**Current Code:**

```cpp
void cleanup() {
    auto cmdDefs = ui_->commandDefinitions();
    auto cmd = cmdDefs->itemById(COMMAND_ID);
    if (cmd) {
        cmd->deleteMe();
    }
    // Continue with other cleanup...
}
```

**Recommended Fix:**

```cpp
void cleanup() {
    try {
        auto cmdDefs = ui_->commandDefinitions();
        if (cmdDefs) {
            auto cmd = cmdDefs->itemById(COMMAND_ID);
            if (cmd && cmd->isValid()) {
                cmd->deleteMe();
            }
        }
    } catch (const std::exception& e) {
        // Log but don't throw - cleanup should be resilient
        logger_->error("Command cleanup failed: " + std::string(e.what()));
    }
    // Continue with other cleanup...
}
```

**Why This Matters:** Cleanup code should be resilient. If one step fails, subsequent cleanup should still execute.

---

### Issue 10: XY-Parallel Plane Restriction (LOW PRIORITY - DOCUMENTATION)

**Location:** `src/adapters/FusionWorkspaceSketchPlane.cpp:97-106`

**Problem:** The code restricts sketch planes to be parallel to the XY plane (i.e., orthogonal to the Z axis). This is a valid design decision for 2D carving workflows but isn't formally documented.

**Current Code:**

```cpp
// Check if normal is parallel to Z axis (0, 0, ±1)
double tolerance = 0.001;
if (std::abs(normal->x()) < tolerance && std::abs(normal->y()) < tolerance &&
    std::abs(std::abs(normal->z()) - 1.0) < tolerance) {
  isValidPlane = true;
}
```

**Recommendation:** The restriction is sensible for carving applications where toolpaths are generated in 2D and then projected onto 3D surfaces. Consider adding a comment or documentation explaining this design rationale.

---

## Action Plan

### High Priority (Fix Soon)

1. **Issue 1:** Add event handler tracking and cleanup
2. **Issue 3:** Add isValid() checks to collection iterations
3. **Issue 7:** Add null checks to API call chains

### Medium Priority (Improve When Convenient)

4. **Issue 2:** Standardize unit conversion calls
5. **Issue 6:** Centralize tolerance constants
6. **Issue 5:** Add revision-based cache invalidation

### Low Priority (Consider for Future)

7. **Issue 8:** Create collection iteration helpers
8. **Issue 9:** Add try-catch to cleanup code
9. **Issue 10:** Document Z-parallel restriction

---

## Testing Recommendations

After implementing fixes, test these scenarios:

1. **Memory:** Load/unload add-in multiple times, monitor memory usage
2. **Undo/Redo:** Create geometry, undo, verify no crashes when accessing invalidated objects
3. **Edge Cases:** Empty sketches, single-point sketches, complex profiles
4. **Unit Conversion:** Verify geometry matches expected dimensions across different operations

---

## Files Reviewed

| File                            | Status   | Notes                       |
| ------------------------------- | -------- | --------------------------- |
| IFusionInterface.h              | ✅ Good  | Clean abstractions          |
| FusionAPIAdapter.h              | ✅ Good  | Proper implementation       |
| FusionSketchCore.cpp            | ⚠️ Minor | Issue 6 (tolerances)        |
| FusionSketch3D.cpp              | ⚠️ Minor | Issue 2 (unit conversion)   |
| FusionSketchConstruction.cpp    | ✅ Good  | Proper isValid() usage      |
| FusionWorkspaceSketchPlane.cpp  | ⚠️ Note  | Issue 10 (Z restriction)    |
| FusionWorkspaceEntityLookup.cpp | ✅ Good  | Correct findEntityByToken() |
| FusionWorkspaceProfile.cpp      | ⚠️ Minor | Issues 3, 5                 |
| FusionWorkspaceCurveSurface.cpp | ⚠️ Minor | Issues 6, 7                 |
| PluginCommandsCreation.cpp      | ⚠️ Minor | Issue 1 (handlers)          |
| PluginCommandsParameters.cpp    | ✅ Good  | Correct parameter handling  |
| PluginInitializer.cpp           | ⚠️ Minor | Issue 9 (cleanup)           |
| ErrorHandler.h/cpp              | ✅ Good  | Consistent error handling   |
| UnitConversion.h                | ✅ Good  | Centralized utilities       |

---

## Implementation Status

| Issue | Description                            | Priority | Status         | Notes                                   |
| ----- | -------------------------------------- | -------- | -------------- | --------------------------------------- |
| 1     | Event Handler Memory Management        | Medium   | ✅ Implemented | Added cleanup in destructors (Dec 2025) |
| 2     | Inconsistent Unit Conversion in 3D     | Low      | ✅ Implemented | Use Utils::mmToFusionLength() in FusionSketch3D.cpp (Dec 2025) |
| 3     | Missing isValid() Checks in Iterations | Medium   | ✅ Implemented | Added checks to 5 files (Dec 2025)      |
| 4     | ValueInput Usage                       | N/A      | No Action      | Already correct                         |
| 5     | Profile Caching Without Invalidation   | Low      | Open           |                                         |
| 6     | Hard-Coded Tolerance Values            | Low      | ✅ Implemented | Added Utils::Tolerance constants (Dec 2025) |
| 7     | Missing Null Checks in API Call Chains | Medium   | ✅ Implemented | Fixed chained call in PluginCommandsImport.cpp (Dec 2025) |
| 8     | size_t vs int in Collection Iteration  | Low      | Open           |                                         |
| 9     | Command Cleanup Order                  | Low      | ✅ Implemented | Added try-catch to ShutdownPlugin (Dec 2025) |
| 10    | XY-Parallel Plane Restriction          | Low      | ✅ Implemented | Documented in README.md (Dec 2025)      |

---

_Review conducted: December 2025_
_Reviewer: Claude (AI Assistant)_
_Documentation reference: fusion_api/OVERVIEW.md_

# Fusion API Usage Improvements Plan

## Priority 1: Refactor Entity Lookup to Use findEntityByToken()

### Problem
The current code manually iterates through all components, bodies, and faces to find entities by their token string. This is O(n×m×k) complexity when `Design.findEntityByToken()` provides O(1) lookup.

### Files Affected
1. `src/adapters/FusionWorkspaceSketchComponent.cpp` - `createSketchInTargetComponent()`
2. `src/adapters/FusionWorkspaceProfileSearch.cpp` - `findProfileByEntityToken()`
3. `src/adapters/FusionWorkspaceSketchPlane.cpp` - `createSketchOnPlane()`

### Implementation Steps

#### Step 1.1: Create a helper function for entity lookup
Add to `FusionAPIAdapter.h` and implement in a new file `FusionWorkspaceEntityLookup.cpp`:

```cpp
// In FusionWorkspace class
private:
  // Direct entity lookup using Fusion API
  Ptr<adsk::core::Base> findEntityByToken(const std::string& entityToken);

  // Get parent component from various entity types
  Ptr<adsk::fusion::Component> getComponentFromEntity(Ptr<adsk::core::Base> entity);
```

#### Step 1.2: Refactor createSketchInTargetComponent()
Replace the ~100 lines of manual iteration with:

```cpp
std::unique_ptr<ISketch> FusionWorkspace::createSketchInTargetComponent(
    const std::string& name, const std::string& surfaceEntityId) {

  Ptr<adsk::fusion::Design> design = app_->activeProduct();
  if (!design) return nullptr;

  Ptr<adsk::fusion::Component> targetComponent = nullptr;

  if (!surfaceEntityId.empty()) {
    // DIRECT LOOKUP - replaces manual iteration
    std::vector<Ptr<adsk::core::Base>> entities = design->findEntityByToken(surfaceEntityId);
    if (!entities.empty()) {
      targetComponent = getComponentFromEntity(entities[0]);
    }
  }

  // Fallback to root component
  if (!targetComponent) {
    targetComponent = design->rootComponent();
  }

  // ... rest of sketch creation
}
```

#### Step 1.3: Refactor findProfileByEntityToken()
Similar simplification - the profile search can use the same pattern.

#### Step 1.4: Refactor createSketchOnPlane()
The construction plane and face lookup can also use `findEntityByToken()`.

### Testing Strategy
- Test with single-component designs
- Test with multi-component designs (the main use case)
- Test with mesh bodies (STL imports)
- Test with designs that have been modified after profile selection (entity token stability)

### Rollback Plan
Keep the old implementation available (commented or in a backup branch) for 1-2 releases in case edge cases emerge.

---

## Priority 2: Add getLastError() for Better Diagnostics

### Problem
API failures return null but we don't capture the specific error message from Fusion.

### Files Affected
All files in `src/adapters/` that call Fusion API methods.

### Implementation Steps

#### Step 2.1: Create error logging helper
```cpp
// In FusionAPIAdapter.h
private:
  void logApiError(const std::string& operation) const;

// Implementation
void FusionWorkspace::logApiError(const std::string& operation) const {
  if (app_) {
    std::string errorMessage;
    int errorCode = app_->getLastError(&errorMessage);
    if (errorCode != 0) {
      LOG_ERROR(operation << " failed: " << errorMessage << " (code: " << errorCode << ")");
    }
  }
}
```

#### Step 2.2: Add to critical failure points
Focus on:
- Sketch creation failures
- Geometry creation failures (lines, arcs, splines)
- Profile extraction failures

---

## Priority 3: Review executeTextCommand Usage

### Problem
`FusionSketchCore.cpp:270-271` uses `executeTextCommand()` to "finish" sketches. This may be unnecessary or have unintended side effects.

### Investigation Needed
- Test if removing these calls affects sketch behavior
- Check if there's a proper API method for flushing sketch changes
- Document why this was added if it's necessary

---

## Priority 4: Improve Mesh Ray Intersection

### Problem
The manual triangle intersection in `FusionWorkspaceCurveSurface.cpp:158-214` has incomplete point-in-triangle testing.

### Implementation Steps
- Implement proper barycentric coordinate testing
- Or find if there's an API method for mesh ray intersection

---

## Priority 5: Consistent isValid() Checking

### Problem
`isValid()` is checked before deletion but not consistently before other operations on potentially stale objects.

### Implementation Steps
- Audit all places where cached Ptr<> objects are used
- Add isValid() checks where objects could have been invalidated

---

## Commit Message Template for Priority 1

```
⚠️ MAJOR CHANGE: Replace manual entity iteration with Design.findEntityByToken()

This commit changes how we locate surfaces/profiles in multi-component designs.

BEFORE: Manually iterated through all components → bodies → faces comparing
        entity tokens (O(n×m×k) complexity, ~150 lines of nested loops)

AFTER:  Direct lookup via Design.findEntityByToken() API method
        (O(1) complexity, ~15 lines)

WHY: The previous approach was working around the API rather than using it
     as intended. findEntityByToken() is specifically designed for this purpose.

⚠️ IF YOU EXPERIENCE ISSUES with surface detection in multi-component designs
   after this change, this commit may need to be reverted. The old approach
   handled some edge cases that may not be obvious.

TEST CASES TO VERIFY:
- [ ] Single component with sketch on XY plane
- [ ] Multi-component with sketch in sub-component
- [ ] Mesh body (STL import) surface detection
- [ ] Design modified after profile selection

Files changed:
- src/adapters/FusionWorkspaceSketchComponent.cpp
- src/adapters/FusionWorkspaceProfileSearch.cpp
- src/adapters/FusionWorkspaceSketchPlane.cpp
- src/adapters/FusionWorkspaceEntityLookup.cpp (new)
```

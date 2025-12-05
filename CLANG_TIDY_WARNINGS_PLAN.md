# Plan: Fix clang-tidy Warnings

## Current State

- 182 warnings across 59 files
- `make lint-tidy` now uses clangd (same as Zed) and runs in ~2 minutes
- Warnings are categorized below by type

## Phase 1: Decide Which Warnings to Keep vs Suppress

### Keep (valuable, should fix):

| Check                                                 | Count | Reason to Keep                           |
| ----------------------------------------------------- | ----- | ---------------------------------------- |
| `unused-includes`                                     | ~80   | Reduces compile time, cleaner code       |
| `performance-unnecessary-value-param`                 | ~20   | Real performance issue with Ptr<> copies |
| `cppcoreguidelines-narrowing-conversions`             | ~10   | Potential bugs with size_t to int        |
| `performance-avoid-endl`                              | ~13   | Real performance issue (endl flushes)    |
| `misc-unused-using-decls`                             | ~10   | Dead code cleanup                        |
| `performance-for-range-copy`                          | ~2    | Unnecessary copies in loops              |
| `bugprone-implicit-widening-of-multiplication-result` | ~2    | Potential overflow bugs                  |
| `cert-err33-c`                                        | ~1    | Ignoring error return values             |
| `readability-else-after-return`                       | ~1    | Style consistency                        |
| `google-readability-casting`                          | ~1    | Prefer C++ casts                         |

### Consider Suppressing (noisy, low value):

| Check                                               | Count | Reason to Consider Suppressing                                                     |
| --------------------------------------------------- | ----- | ---------------------------------------------------------------------------------- |
| `cppcoreguidelines-pro-bounds-constant-array-index` | ~25   | TriArc uses fixed-size arrays with computed indices - this is intentional and safe |
| `readability-isolate-declaration`                   | ~2    | Minor style preference                                                             |
| `readability-use-std-min-max`                       | ~1    | Minor style preference                                                             |

### Decision needed:

- `cppcoreguidelines-pro-bounds-constant-array-index`: The TriArc shape uses `std::array<Point2D, 3>` and accesses with computed indices. This is safe because indices are always 0-2. Suppress or refactor?

## Phase 2: Fix Unused Includes (~80 warnings)

Files to clean up (in order of most warnings):

1. `src/core/PluginManager*.cpp` - multiple unused headers
2. `src/adapters/FusionWorkspace*.cpp` - iostream, algorithm, etc.
3. `src/geometry/TriArc*.cpp` - algorithm, functional, limits
4. `src/geometry/MedialAxisProcessor*.cpp` - iomanip, iostream, streambuf
5. `src/geometry/SVGGenerator*.cpp` - algorithm, regex, iomanip

Approach: Remove unused includes one file at a time, rebuild to verify.

## Phase 3: Fix Performance Warnings (~35 warnings)

### 3a: `performance-unnecessary-value-param` (~20)

Change `Ptr<T> param` to `const Ptr<T>& param` in:

- `src/utils/ErrorHandler.cpp` (3 functions)
- `src/utils/UIParameterHelper.cpp`
- `src/utils/FusionComponentTraverser.cpp` (4 functions)
- `src/adapters/FusionSketchCore.cpp` (2 params)
- `src/adapters/FusionWorkspaceCurveGeometry.cpp`
- `src/adapters/FusionWorkspaceProfileGeometry.cpp`
- `src/adapters/FusionAPIFactory.cpp` (2 params)
- `src/adapters/FusionWorkspaceEntityLookup.cpp`
- `src/adapters/FusionLogger.cpp`
- `src/adapters/FusionWorkspaceSketchBasic.cpp`
- `src/commands/PluginCommandsParameters.cpp` (2 functions)
- `src/commands/PluginCommandsValidation.cpp` (2 functions)
- `src/commands/PluginCommandsParametersSelection.cpp`
- `src/commands/PluginCommandsGeometryMain.cpp`
- `src/geometry/VCarveCalculatorSurface.cpp`

### 3b: `performance-avoid-endl` (~13)

Replace `std::endl` with `'\n'` in:

- `src/adapters/FusionLogger.cpp` (13 occurrences)

### 3c: `performance-for-range-copy` (~2)

Change loop variable to const reference in:

- `src/utils/FusionComponentTraverser.cpp`

## Phase 4: Fix Narrowing Conversions (~10 warnings)

Change `int` loop variables to `size_t` or use `static_cast<int>()` explicitly:

- `src/adapters/FusionSketchCore.cpp:128`
- `src/adapters/FusionWorkspaceProfile.cpp:134`
- `src/adapters/FusionWorkspaceProfileGeometry.cpp:151`
- `src/commands/PluginCommandsParametersSelection.cpp:28,171`
- `src/geometry/Leaf.cpp:197`
- `src/geometry/TriArcSketch.cpp:132`
- `src/geometry/VCarveCalculatorOptimization.cpp:69`

## Phase 5: Fix Misc Warnings (~15 warnings)

### 5a: `misc-unused-using-decls` (~10)

Remove unused `using` declarations in:

- `src/core/PluginInitializer.cpp` (4 using decls)
- `src/geometry/Leaf.cpp` (3 using decls)
- `src/geometry/TriArcSketch.cpp` (1 using decl)
- `src/geometry/TriArcGeometry.cpp` (2 using decls)
- `src/geometry/ShapeFactory.cpp` (1 using decl)

### 5b: Other fixes

- `src/adapters/FusionLogger.cpp:135` - Handle return value (cert-err33-c)
- `src/adapters/FusionLogger.cpp:121,147` - Fix widening multiplication
- `src/commands/PluginCommandsGeometryChaining.cpp:80` - Remove else after break
- `src/commands/PluginCommandsGeometryMain.cpp:176` - Split declaration
- `src/commands/PluginCommandsGeometryMain.cpp:216` - Use static_cast

## Phase 6: Suppress or Refactor Array Index Warnings (~25 warnings)

Decision: Either:

1. **Suppress** `cppcoreguidelines-pro-bounds-constant-array-index` in `.clangd` and `.clang-tidy`
2. **Refactor** TriArc to use `.at()` instead of `[]` (adds bounds checking overhead)
3. **Add comments** with NOLINT to specific lines

Files affected:

- `src/geometry/TriArcCore.cpp`
- `src/geometry/TriArcSketch.cpp`
- `src/geometry/TriArcGeometry.cpp`
- `src/geometry/SVGGeneratorShapes.cpp`
- `src/geometry/ShapeFactory.cpp`

## Execution Order

1. **Phase 1**: Make decisions on suppressions (update `.clangd` and `.clang-tidy`)
2. **Phase 5b**: Quick misc fixes (5 files, small changes)
3. **Phase 4**: Narrowing conversions (8 files)
4. **Phase 3**: Performance warnings (15+ files)
5. **Phase 5a**: Unused using declarations (5 files)
6. **Phase 2**: Unused includes (15+ files) - do last as most tedious
7. **Phase 6**: Array index decision

## Estimated Effort

- Phase 1: 10 minutes (decision making)
- Phases 2-6: 2-3 hours total

## Success Criteria

- `make lint-tidy` reports 0 warnings
- Zed shows 0 warnings
- All tests pass
- Plugin builds and installs successfully

---

## Task List for Execution

### Phase 1: Configuration Decisions

- [ ] Decide on `cppcoreguidelines-pro-bounds-constant-array-index` (suppress vs refactor)
- [ ] Decide on `readability-isolate-declaration` (suppress vs fix)
- [ ] Decide on `readability-use-std-min-max` (suppress vs fix)
- [ ] Update `.clangd` and `.clang-tidy` with suppressions

### Phase 5b: Quick Misc Fixes (5 files)

- [ ] `FusionLogger.cpp:135` - Handle freopen return value (cert-err33-c)
- [ ] `FusionLogger.cpp:121,147` - Fix widening multiplication with static_cast
- [ ] `PluginCommandsGeometryChaining.cpp:80` - Remove else after break
- [ ] `PluginCommandsGeometryMain.cpp:176` - Split multiple declarations
- [ ] `PluginCommandsGeometryMain.cpp:216` - Use static_cast instead of C-style cast

### Phase 4: Narrowing Conversions (8 files)

- [ ] `FusionSketchCore.cpp:128` - Fix size_t to int
- [ ] `FusionWorkspaceProfile.cpp:134` - Fix size_t to int
- [ ] `FusionWorkspaceProfileGeometry.cpp:151` - Fix size_t to int
- [ ] `PluginCommandsParametersSelection.cpp:28,171` - Fix size_t to int (2 locations)
- [ ] `Leaf.cpp:197` - Fix size_t to int
- [ ] `TriArcSketch.cpp:132` - Fix size_t to int
- [ ] `VCarveCalculatorOptimization.cpp:69` - Fix size_t to difference_type

### Phase 3a: Performance - Unnecessary Value Params (~20 warnings)

- [ ] `ErrorHandler.cpp` - Change 3 function params to const ref
- [ ] `UIParameterHelper.cpp` - Change inputs param to const ref
- [ ] `FusionComponentTraverser.cpp` - Change 4 params to const ref
- [ ] `FusionSketchCore.cpp` - Change app, sketch params to const ref
- [ ] `FusionWorkspaceCurveGeometry.cpp` - Change sketchCurve to const ref
- [ ] `FusionWorkspaceProfileGeometry.cpp` - Change profile to const ref
- [ ] `FusionAPIFactory.cpp` - Change app, ui to const ref
- [ ] `FusionWorkspaceEntityLookup.cpp` - Change entity to const ref
- [ ] `FusionLogger.cpp` - Change ui to const ref
- [ ] `FusionWorkspaceSketchBasic.cpp` - Change app to const ref
- [ ] `PluginCommandsParameters.cpp` - Change 2 inputs params to const ref
- [ ] `PluginCommandsValidation.cpp` - Change curve, selectionInput to const ref
- [ ] `PluginCommandsParametersSelection.cpp` - Change inputs to const ref
- [ ] `PluginCommandsGeometryMain.cpp` - Change profile to const ref
- [ ] `VCarveCalculatorSurface.cpp` - Change surfaceQuery to const ref

### Phase 3b: Performance - Avoid endl (13 warnings)

- [ ] `FusionLogger.cpp` - Replace all std::endl with '\n' (13 occurrences)

### Phase 3c: Performance - For Range Copy (2 warnings)

- [ ] `FusionComponentTraverser.cpp` - Change loop variable to const ref

### Phase 5a: Unused Using Declarations (10 warnings)

- [ ] `PluginInitializer.cpp` - Remove 4 unused using decls
- [ ] `Leaf.cpp` - Remove 3 unused using decls (distance, midpoint, perpendicular)
- [ ] `TriArcSketch.cpp` - Remove 1 unused using decl (distance)
- [ ] `TriArcGeometry.cpp` - Remove 2 unused using decls (distance, midpoint)
- [ ] `ShapeFactory.cpp` - Remove 1 unused using decl (distance)

### Phase 2: Unused Includes (~80 warnings)

- [ ] `PluginManagerCore.cpp` - Remove 9 unused includes
- [ ] `PluginManagerImport.cpp` - Remove 4 unused includes
- [ ] `PluginManagerLegacyPaths.cpp` - Remove 5 unused includes
- [ ] `PluginManagerUtils.cpp` - Remove 3 unused includes
- [ ] `PluginManagerPathsCore.cpp` - Remove 5 unused includes
- [ ] `PluginManagerPathsVisualization.cpp` - Remove 6 unused includes
- [ ] `PluginInitializer.cpp` - Remove 1 unused include
- [ ] `PluginManagerVCarve.cpp` - Remove 5 unused includes
- [ ] `ErrorHandler.cpp` - Remove 1 unused include
- [ ] `UIParameterHelper.cpp` - Remove 1 unused include
- [ ] `FusionWorkspaceSketchComponent.cpp` - Remove 1 unused include
- [ ] `FusionSketch3D.cpp` - Remove 4 unused includes
- [ ] `FusionSketchCore.cpp` - Remove 2 unused includes
- [ ] `FusionWorkspaceCurveGeometry.cpp` - Remove 5 unused includes
- [ ] `FusionWorkspaceSketchPlane.cpp` - Remove 1 unused include
- [ ] `FusionWorkspaceSketchBasic.cpp` - Remove 5 unused includes
- [ ] `PluginCommandsParameters.cpp` - Remove 1 unused include
- [ ] `MedialAxisProcessorCore.cpp` - Remove 5 unused includes
- [ ] `Leaf.cpp` - Remove 1 unused include
- [ ] `VCarveCalculatorSurface.cpp` - Remove 2 unused includes
- [ ] `MedialAxisProcessorVoronoi.cpp` - Remove 4 unused includes
- [ ] `TriArcCore.cpp` - Remove 4 unused includes
- [ ] `TriArcSketch.cpp` - Remove 4 unused includes
- [ ] `SVGGeneratorCore.cpp` - Remove 1 unused include
- [ ] `TriArcGeometry.cpp` - Remove 2 unused includes
- [ ] `VCarveCalculatorCore.cpp` - Remove 1 unused include
- [ ] `VCarveCalculatorOptimization.cpp` - Remove 1 unused include
- [ ] `SVGGeneratorComparator.cpp` - Remove 2 unused includes
- [ ] `SVGGeneratorShapes.cpp` - Remove 5 unused includes
- [ ] `ShapeFactory.cpp` - Remove 1 unused include

### Phase 6: Array Index Warnings (~25 warnings)

- [ ] Apply decision from Phase 1 (suppress globally or add NOLINT comments)

### Final Verification

- [ ] Run `make lint-tidy` - verify 0 warnings
- [ ] Run `make test` - verify all tests pass
- [ ] Run `make install` - verify plugin builds and installs

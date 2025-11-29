# Carving Fusion - Code Refactoring Plan

**Document Version**: 1.0  
**Date**: 2025-11-28  
**Status**: Proposed  
**Target**: Improve code quality without adding new features

**Important**: This plan should be executed in conjunction with guidance from both `CLAUDE.md` and `AGENTS.md`. If conflicts arise between this plan and existing documentation, consult the user before proceeding.

---

## Executive Summary

This refactoring plan addresses technical debt and brings the codebase into full compliance with its own documented quality standards. The codebase is well-architected with 287+ tests, but requires structural improvements to meet the 350-line file limit, resolve TODOs, and enhance maintainability.

---

## Refactoring Items - Quick Reference

| # | Item | Priority | Status | Short Description |
|---|------|----------|--------|-------------------|
| 1 | File Size Compliance | **P0** | Planned | Split 5 files exceeding 350-line limit (534, 389, 381, 350, 349 lines) |
| 2 | Resolve High-Priority TODOs | **P0** | Planned | Fix HelloWorldCommand, ErrorHandler UI integration, dialog implementations |
| 3 | Mock Reorganization | **P1** | Planned | Centralize mock implementations from tests/ to src/adapters/mock/ |
| 4 | Header Include Cleanup | **P1** | Planned | Standardize include paths to use clean paths (not ../../ prefixes) |
| 5 | Error Handling Consolidation | **P1** | Planned | Expand ErrorHandler, add error codes, UI integration |
| 6 | Compiler Warning Cleanup | **P2** | Planned | Fix all warnings with -Wall -Wextra -Wpedantic -Werror |
| 7 | Logging System Refinement | **P2** | Planned | Replace macros with template functions, add structured logging |
| 8 | Const-Correctness Audit | **P2** | Planned | Review and fix const qualifier usage across codebase |
| 9 | Include Guard Modernization | **P2** | Planned | Migrate remaining legacy include guards to `#pragma once` |
| 10 | Test Coverage Expansion | **P3** | Planned | Add tests for utils/, adapter error paths, performance benchmarks |
| 11 | Memory Management Audit | **P3** | Planned | Review lifetimes, weak_ptr usage, ownership patterns |
| 12 | Modern C++14 Feature Adoption | **P3** | Planned | Leverage constexpr, static_assert, range-based for where applicable |

**Priority Legend**: P0 = Critical (Must complete), P1 = High (Should complete), P2 = Medium (Nice to have), P3 = Low (Future improvement)

---

## Detailed Refactoring Items

### P0 - Critical Items (Must Complete)

#### 1. File Size Compliance

**Problem**: Five files violate the documented 350-line maximum:
- `src/commands/PluginCommandsParameters.cpp` - 534 lines (52% over limit)
- `src/core/PluginInitializer.cpp` - 389 lines (11% over limit)
- `src/commands/PluginCommandsGeometry.cpp` - 381 lines (9% over limit)
- `src/adapters/FusionWorkspaceProfile.cpp` - 350 lines (at limit)
- `src/geometry/MedialAxisProcessorCore.cpp` - 349 lines (at limit)

**Solution**: Follow established splitting pattern (e.g., MedialAxisProcessorCore.cpp already split from MedialAxisProcessor.cpp):

**For PluginCommandsParameters.cpp:**
```
PluginCommandsParameters.cpp →
  - PluginCommandsParametersSetup.cpp (dialog creation)
  - PluginCommandsParametersValidation.cpp (input validation)
  - PluginCommandsParametersDefaults.cpp (default values & helpers)
```

**For PluginInitializer.cpp:**
```
PluginInitializer.cpp →
  - PluginInitializerStartup.cpp (initialization logic)
  - PluginInitializerCommands.cpp (command registration)
  - PluginInitializerCleanup.cpp (shutdown & resource cleanup)
```

**For PluginCommandsGeometry.cpp:**
```
PluginCommandsGeometry.cpp →
  - PluginCommandsGeometryExtraction.cpp (profile extraction)
  - PluginCommandsGeometryValidation.cpp (geometry validation)
  - PluginCommandsGeometryProcessing.cpp (geometry processing)
```

**Rationale**: This is explicitly required by AGENTS.md line 162 and CLAUDE.md line 275. Violations make `make lint` fail and indicate architectural debt.

**Verification**:
- All new files ≤ 350 lines
- All existing tests pass without modification
- No public API changes
- `make lint` passes without file size warnings

---

#### 2. Resolve High-Priority TODOs

**Found 21 TODOs/FIXMEs**: Several are quick wins with outsized impact.

**High Priority TODOs (Must Fix)**:

1. **src/core/PluginInitializer.cpp:11**
   ```cpp
   // TODO(dev): Fix HelloWorldCommand API issues
   ```
   **Action**: Either fix the API issues or remove the dead code and associated header. Having disabled code in production is a code smell.

2. **src/utils/ErrorHandler.cpp:38,53**
   ```cpp
   // TODO(dev): Show message to user via UI
   ```
   **Action**: Integrate ErrorHandler with IUserInterface to display errors to users instead of just logging them. Add `ErrorHandler::reportToUser()` method.

3. **src/adapters/FusionLogger.cpp:208,219,250**
   ```cpp
   // TODO(ui): Implement confirmation dialog
   // For now, implement as simple message box - TODO(ui): Replace with proper
   ```
   **Action**: Complete the stubbed dialog implementations or remove if no longer needed.

**Low Priority (Cleanup)**:
- `src/adapters/FusionWorkspaceProfile.cpp:34` - Remove indicator file creation (debug code)
- `src/core/PluginManagerUtils.cpp:19,25` - Remove placeholder comments
- Various `// TODO(developer): Add implementation` - Remove or implement

**Rationale**: TODOs in production code indicate incomplete work and technical debt. Per AGENTS.md best practices, code should be complete before commit.

**Note**: Based on impression from reviewing the code, most of these TODOs appear to be leftover cruft from development iterations rather than intentional future work. However, they should still be cleaned up as part of the refactoring effort—either by implementing them properly if still relevant, or removing them if obsolete.

**Verification**:
- Zero TODO/FIXME/XXX comments in production source (src/)
- Tests still pass
- HelloWorldCommand either fully functional or completely removed

---

### P1 - High Priority (Should Complete)

#### 3. Mock Reorganization

**Problem**: Mock implementations are scattered across test directories:
- `tests/adapters/MockAdapters.h` (720 lines - large!)
- `tests/mocks/MockLogging.cpp`
- No clear separation from production code

**Solution**: Create dedicated mock directory following production structure:
```
src/adapters/mock/
  - MockLogger.h/.cpp
  - MockUserInterface.h/.cpp
  - MockWorkspace.h/.cpp
  - MockFactory.h/.cpp
  - MockAdapters.h (refactored to just include individual mocks)
```

Create corresponding include directory:
```
include/adapters/mock/
  - MockInterfaces.h (forward declarations)
```

**Rationale**: 
- Aligns with AGENTS.md "mock implementations for testing" principle
- Makes mocks reusable across test suites
- Follows CLAUDE.md guidance on adapter organization
- Reduces test compilation times

**Verification**:
- All tests pass with new mock locations
- Mocks can be included individually without pulling in entire 720-line file
- Mock implementations mirror production interface structure

---

#### 4. Header Include Path Cleanup

**Problem**: Inconsistent include patterns across codebase:
```cpp
// Pattern 1 (common, but verbose):
#include "../../include/geometry/Shape.h"

// Pattern 2 (local headers):
#include "PluginCommands.h"

// Pattern 3 (clean paths exist):
#include <geometry/Shape.h>  // CMake sets include path
```

**Solution**: Standardize to clean, non-relative paths:
- External libraries: `#include <library/header.h>`
- Project public headers: `#include <geometry/Shape.h>` or `#include "geometry/Shape.h"`
- Local headers: `#include "IFusionInterface.h"` (within same dir)

**Rationale**:
- Better compile-time performance (fewer directory searches)
- Improved readability
- Easier refactoring (move files without updating paths)
- Follows Google C++ Style Guide recommendations

**Implementation**:
1. Update CMakeLists.txt to ensure `include/` is in system include path
2. Globally replace `"../../include/` with `<` or `"` as appropriate
3. Verify all compilations succeed

**Verification**:
- `make build` succeeds with no changes to CMakeLists.txt includes
- `grep -r "../../include" src/` returns no results
- All tests pass

---

#### 5. Error Handling Consolidation

**Problem**: Error handling inconsistent across codebase:
- Many functions return bool for success/failure
- Some throw exceptions
- Some log errors and continue
- No unified error codes or user feedback mechanism

**Solution**: Expand `ErrorHandler` system:

```cpp
// Add to include/utils/ErrorHandler.h
enum class ErrorCode {
  SUCCESS = 0,
  INVALID_INPUT = 100,
  FILE_NOT_FOUND = 101,
  PARSER_ERROR = 200,
  MEDIAL_AXIS_FAILED = 300,
  VCARVE_FAILED = 400,
  FUSION_API_ERROR = 500,
  INTERNAL_ERROR = 999
};

class ErrorHandler {
 public:
  static ErrorCode getLastError() const;
  static std::string getErrorMessage(ErrorCode code);
  static void reportToUser(const std::string& message);  // Uses IUserInterface
  static void clearError();
  
  // RAII error context
  class ErrorContext {
   public:
    explicit ErrorContext(const std::string& operation);
    ~ErrorContext();
  };
};
```

**Rationale**: 
- Centralizes error handling logic
- Provides better user experience (consistent error messages)
- Easier debugging with error codes
- RAII pattern ensures error state cleanup

**Verification**:
- All existing error handling paths updated
- Error messages displayed to user via UI
- Tests for error conditions pass
- No regression in error handling behavior

---

### P2 - Medium Priority (Nice to Have)

#### 6. Compiler Warning Cleanup

**Problem**: Unknown current warning level (not using -Werror)

**Solution**: Enable strict compiler warnings and fix all issues:
```bash
cmake -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter"
```

**Expected Issues to Fix**:
- Unused parameters (add `[[maybe_unused]]` or remove)
- Missing `override` keyword on virtual methods
- Implicit type conversions (use `static_cast`)
- Sign comparison warnings (use consistent types)
- Missing field initializers

**Rationale**: Prevents bugs, enforces code quality, mandatory for pre-commit workflow `make pre-commit` (per AGENTS.md line 172)

**Verification**:
- `make build` succeeds with strict warning flags
- Zero warnings emitted
- All tests pass

---

#### 7. Logging System Refinement

**Problem**: Current macro-based system works but has issues:
- Runtime checks despite compile-time macros
- Macro bloat in debug builds
- No structured logging support
- Type safety issues

**Solution**: Template-based logging (C++14 compatible):

```cpp
// Replace macros with templates
template<typename... Args>
void logDebug(const std::string& fmt, Args&&... args) {
#ifdef DEBUG
  if (isLevelEnabled(LogLevel::DEBUG)) {
    logFormatted(LogLevel::DEBUG, fmt, std::forward<Args>(args)...);
  }
#endif
}

// Usage stays similar:
logDebug("Processing {} shapes", shapes.size());
```

**Benefit**:
- Zero overhead when disabled (template doesn't instantiate)
- Type-safe formatting
- Better IDE support (auto-complete)
- Easier to extend with structured fields

**Verification**:
- Debug build: Full logging works
- Release build: Zero overhead (verified via disassembly check)
- All existing log statements compile without change
- Performance tests show no regression

---

#### 8. Const-Correctness Audit

**Problem**: Inconsistent const usage:
- Methods that don't modify state not marked `const`
- Parameters passed by non-const reference unnecessarily
- Mutable state overused instead of proper design

**Action**: Systematic review:
1. Identify all accessor methods → add `const`
2. Review parameters → make const references where applicable
3. Check for `[[nodiscard]]` opportunities on query methods
4. Review `mutable` usage → refactor if used to bypass const

**Rationale**:
- Prevents bugs (enforces immutability guarantees)
- Enables compiler optimizations
- Better self-documenting code
- Required for thread safety in future

**Example**:
```cpp
// Before:
std::string getName();

// After:
[[nodiscard]] const std::string& getName() const noexcept;
```

**Verification**:
- All tests compile and pass
- No const_cast needed in implementation
- Code review: all const usage justified

---

#### 9. Include Guard Modernization

**Problem**: Mixed include guard styles:
- 90% use `#pragma once` (modern, preferred)
- 10% use `#ifndef` legacy guards (e.g., `logging.h`)

**Solution**: Migrate remaining legacy guards:
```cpp
// Before:
#ifndef INCLUDE_UTILS_LOGGING_H_
#define INCLUDE_UTILS_LOGGING_H_
// ... content ...
#endif

// After:
#pragma once
// ... content ...
```

**Rationale**: Consistency, brevity, less error-prone (no name collisions)

**Verification**:
- All headers use #pragma once
- No header included multiple times (checked via #pragma once simulation)
- Build succeeds on all platforms

---

### P3 - Lower Priority (Future Improvements)

#### 10. Test Coverage Expansion

**Current Gaps**:
- `src/utils/` directory has minimal test coverage
- Adapter layer error paths not tested
- No performance regression tests
- Visual tests exist but not automated

**Add Tests For**:
```
tests/utils/
  - test_ErrorHandler.cpp (all error paths)
  - test_FusionComponentTraverser.cpp
  - test_UIParameterHelper.cpp
  - test_UnitConversion.cpp

tests/adapters/
  - test_FusionAPIAdapter.cpp (error injection)
  - test_AdapterErrorHandling.cpp

tests/performance/
  - benchmark_MedialAxis.cpp (OpenVoronoi timing)
  - benchmark_PolygonExtraction.cpp
```

**Rationale**: Per AGENTS.md lines 142-150, testing strategy should include comprehensive coverage. Current gaps risk regressions.

**Verification**:
- Coverage report shows >90% for utils/
- All failure paths tested in adapters
- Performance benchmarks establish baselines
- Total test count increases from 287+ to 320+

---

#### 11. Memory Management Audit

**Problem Areas Identified**:
- Adapters layer uses `std::weak_ptr` in some places
- Fusion API pointer lifetimes unclear
- Potential circular dependencies in command handlers

**Action**:
1. Review all weak_ptr usage → document why needed
2. Audit Fusion API pointers → add lifetime validation
3. Check for cycles in handler → manager → handler
4. Consider custom deleters for Fusion objects

**Rationale**: Avoid crashes from dangling pointers, document ownership clearly (RAII principle per AGENTS.md line 250)

**Verification**:
- No use-after-free in valgrind testing
- Clear ownership documented in comments
- Weak_ptr usage justified and tested

---

#### 12. Modern C++14 Feature Adoption

**Current State**: C++14 compatible but not leveraging features:
- Limited constexpr usage
- No type traits
- Hand-written loops instead of algorithms

**Opportunities** (C++14 only, no C++17):
```cpp
// 1. Add constexpr to constants
constexpr double DEFAULT_TOLERANCE = 0.25;

// 2. Use static_assert for schema validation
static_assert(SCHEMA_VERSION_MAJOR == 2, "Schema must be v2.0");

// 3. Range-based for with auto (already used some places, expand)
for (const auto& point : results.chains[i]) { ... }

// 4. [[deprecated]] on legacy methods
[[deprecated("Use computeMedialAxisFromPolygon instead")]]
void computeMedialAxisFromShape(...);
```

**Rationale**: Better compile-time checks, self-documenting code, prepare for future modernization

**Verification**:
- All code compiles with -std=c++14 (no newer features)
- static_asserts trigger at compile time if violated
- Tests for deprecated methods still pass
- No performance regression

---

## Implementation Approach

### Phase 1: Critical (Week 1)
1. Split files exceeding 350-line limit
2. Resolve high-priority TODOs
3. Run full test suite after each change
4. Verify `make lint` passes

### Phase 2: High Priority (Week 2)
1. Reorganize mock implementations
2. Clean up header include paths
3. Consolidate error handling
4. Address compiler warnings
5. Pre-commit: `make pre-commit` must pass

### Phase 3: Medium Priority (Week 3)
1. Refine logging system
2. Audit const-correctness
3. Modernize include guards
4. Performance profiling

### Phase 4: Lower Priority (Ongoing)
1. Expand test coverage
2. Complete memory audit
3. Adopt moderate C++14 features

---

## Verification & Success Criteria

### Pre-Refactoring Baseline
- [ ] Document current metrics:
  - Files > 350 lines: __
  - TODO/FIXME count: __
  - Test count: __
  - Coverage %: __
  - Compiler warnings: __

### Post-Refactoring Requirements
- [ ] **Zero files exceed 350 lines** (`wc -l src/**/*.cpp | grep -v "^ *1..$"`)
- [ ] **Zero TODO/FIXME in src/** (`grep -r "TODO\|FIXME" src/` returns empty)
- [ ] **All tests pass** (`make test` returns 0)
- [ ] **Zero warnings** with `-Wall -Wextra -Wpedantic -Werror`
- [ ] **make lint passes** (file size, style, C++14 compatibility)
- [ ] **make pre-commit passes** (comprehensive quality check)
- [ ] **No API changes** (all existing code using plugin still compiles)
- [ ] **Coverage maintained or improved** (>85% core, >90% utils)

### Continuous Verification
- Run `make test` after each file split
- Run `make lint` after each major change
- Use `git bisect` friendly commits (one logical change per commit)
- Maintain green build throughout refactoring

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| File splitting introduces bugs | High | Run full test suite after each split; keep focusing on one split per commit |
| Loss of git history | Low | Use `git log --follow` to track history; add detailed commit messages |
| Refactoring breaks tests | Medium | TDD approach: tests first, ensure they pass, then refactor |
| Undocumented behavior changes | Medium | Maintain API compatibility; any behavior change must be explicit |
| Schedule overrun | Low | Prioritize P0 items; P3 items can be deferred if needed |

---

## Documentation Updates

After refactoring, update:
1. **CLAUDE.md**: Add notes about mock organization if structure changes
2. **AGENTS.md**: No changes needed (already documents standards)
3. **README.md**: Update if directory structure changes
4. **This file**: Mark items as completed, add lessons learned

---

## Effort Estimates

| Phase | Effort | Risk Level |
|-------|--------|------------|
| Phase 1 (P0) | 16 hours | Low |
| Phase 2 (P1) | 16 hours | Low |
| Phase 3 (P2) | 12 hours | Low |
| Phase 4 (P3) | 8 hours | Low |
| **Contingency (20%)** | 10 hours | - |
| **Total** | **~62 hours** | **Low** |

*Note: Estimates assume familiarity with codebase and test suite*

---

## Compliance with Existing Documentation

This plan directly addresses requirements from project documentation:

**From AGENTS.md**:
- ✓ File size limit: 350 lines maximum (P0 Item 1)
- ✓ Code quality: `make lint` must pass (all items)
- ✓ Pre-commit workflow: `make pre-commit` (verification criteria)
- ✓ No Fusion API in tests: Mock reorganization (Item 3)
- ✓ C++14 standard: All changes maintain compatibility

**From CLAUDE.md**:
- ✓ 350 line file limit: Refactored (Item 1)
- ✓ Testing philosophy: Mock organization (Item 3)
- ✓ Quality standards: Error handling, logging (Items 5, 7)
- ✓ Development workflow: TDD approach, pre-commit checks

**Conflict Resolution**: Any conflict between this plan and existing docs should be resolved by consulting the user. Current interpretation follows the principle: **Documented standards > Refactoring plan > Individual preference**.

---

## Appendix

### A. Files Exceeding Line Limit - Detailed

```bash
$ wc -l src/**/*.cpp | sort -n | tail -10
534 src/commands/PluginCommandsParameters.cpp  ← Split (184 lines over)
389 src/core/PluginInitializer.cpp             ← Split (39 lines over)
381 src/commands/PluginCommandsGeometry.cpp    ← Split (31 lines over)
350 src/adapters/FusionWorkspaceProfile.cpp    ← At limit
349 src/geometry/MedialAxisProcessorCore.cpp   ← At limit
```

### B. TODO Locations - Full List

**Critical**:
- src/core/PluginInitializer.cpp:11 - HelloWorldCommand
- src/utils/ErrorHandler.cpp:38,53 - UI error reporting
- src/adapters/FusionLogger.cpp:208,219,250 - Dialog implementations

**Cleanup**:
- src/adapters/FusionWorkspaceProfile.cpp:34 - Remove debug code
- src/core/PluginManagerUtils.cpp:19,25 - Remove placeholders
- Various TODO(developer) in PluginCommandsCreation.cpp

### C. Mock Dependencies

**Current**: tests/adapters/MockAdapters.h (720 lines)  
**Problem**: Large file, doesn't mirror production structure, includes all mocks even when only one needed

**Target**: src/adapters/mock/*.h (individual files, each <200 lines)

---

**End of Refactoring Plan**

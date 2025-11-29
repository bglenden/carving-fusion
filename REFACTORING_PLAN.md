# Carving Fusion - Refactoring Roadmap

**Last Updated**: 2025-11-29

## Completed Refactoring

### Critical Infrastructure (P0) - All Complete

| Item | Description | Date |
|------|-------------|------|
| File Size Compliance | Split 5 oversized files into 13 compliant files (all ≤ 350 lines) | 2025-11-28 |
| TODO Cleanup | Resolved 6 actionable TODOs, ErrorHandler UI integration | 2025-11-28 |
| Header Include Cleanup | CMake + 27 files updated to clean relative paths | 2025-11-28 |
| CPP-includes-CPP Anti-pattern | Eliminated all 10 aggregator files, 31 sub-files now compile independently | 2025-11-29 |
| Error Handling Consolidation | ErrorHandler now shows Fusion 360 dialogs | 2025-11-28 |

### Code Quality (P1) - All Complete

| Item | Description | Date |
|------|-------------|------|
| API Improvements | Replaced manual entity iteration with `Design.findEntityByToken()`, added `getLastError()` diagnostics | 2025-11-29 |
| clang-tidy Phase 1 | Replaced `using namespace` with specific using declarations | 2025-11-29 |
| clang-tidy Phase 2 | Added default member initializers across codebase | 2025-11-29 |
| clang-tidy Phase 3 | Fixed empty catch blocks, added virtual destructor defaults | 2025-11-29 |
| clang-tidy Phase 4 | Added `noexcept` to move operations in `Optional.h` | 2025-11-29 |
| try/catch Cleanup | Removed unnecessary try/catch around Fusion API calls (48 files, -361 lines) | 2025-11-29 |

---

## Remaining Work

### P1 - High Priority

#### Mock Reorganization
**Problem**: Mock implementations scattered across test directories (tests/adapters/MockAdapters.h is 720 lines)

**Solution**: Create dedicated mock directory:
```
src/adapters/mock/
  - MockLogger.h/.cpp
  - MockUserInterface.h/.cpp
  - MockWorkspace.h/.cpp
  - MockFactory.h/.cpp
```

### P2 - Medium Priority

| Item | Description |
|------|-------------|
| Compiler Warning Cleanup | Enable `-Wall -Wextra -Wpedantic -Werror` and fix all warnings |
| Logging System Refinement | Replace macros with template functions for type safety |
| Const-Correctness Audit | Add `const` to accessor methods, review `mutable` usage |
| Include Guard Modernization | Migrate remaining `#ifndef` guards to `#pragma once` |

### P3 - Lower Priority

| Item | Description |
|------|-------------|
| Test Coverage Expansion | Add tests for utils/, adapter error paths, performance benchmarks |
| Memory Management Audit | Review lifetimes, weak_ptr usage, ownership patterns |
| Modern C++14 Features | Leverage constexpr, static_assert where applicable |

---

## Current TODOs (Non-Critical)

**10 remaining TODOs** - all optional enhancements:
- 6 in `FusionLogger.cpp`: UI enhancement opportunities (confirmation dialogs, sketch selection)
- 4 in `PluginManagerPathsVisualization.cpp`: Debug logging placeholders

---

## Verification Checklist

Before any commit:
- [ ] `make test` - All 287 tests pass
- [ ] `make lint` - Zero errors, all files ≤ 350 lines
- [ ] `make install` - Plugin builds and installs

---

## Notes

### Fusion 360 API Characteristics
- **Does NOT throw exceptions** - uses null `Ptr<>` returns and boolean return values
- Use `Application::getLastError()` for error details
- Entity lookup: Use `Design.findEntityByToken()` for O(1) lookup

### File Organization
- 350 line limit enforced via `make lint`
- Split files by logical functionality (Core, Validation, Utils, etc.)
- Each `.cpp` compiles to its own object file (no aggregator pattern)

# CLAUDE.md

## CRITICAL COMMIT REQUIREMENTS

**All three conditions MUST be met before any commit:**

1. ✅ **All tests pass** - Run `make test` or `./build/tests/chip_carving_tests`
2. ✅ **Lint is clean** - Run `make lint` and verify `Total errors found: 0`
3. ✅ **Plugin builds and installs** - Run `make install` successfully

**Never commit if any condition fails.** Revert changes or fix issues first.

---

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

The Fusion 360 CNC Chip Carving Plugin is a mature C++ add-in that converts 2D chip carving design shapes into 3D CNC toolpaths using medial axis computation. The plugin generates V-carve toolpaths that preserve traditional chip carving aesthetics while enabling precision CNC fabrication.

**Version**: Follows semantic versioning (MAJOR.MINOR.PATCH)

## Common Development Commands

### Build and Install

```bash
# Standard build workflow (from project root)
mkdir build && cd build
cmake ..
make                    # Build everything
make install           # Install to Fusion 360

# Debug build with full symbols
cmake -DCMAKE_BUILD_TYPE=Debug .. && make

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Clean rebuild
make clean             # Clean build artifacts
# or for complete clean:
rm -rf build && mkdir build && cd build && cmake ..
```

### Testing Commands

```bash
# Run all tests (287+ unit tests)
make test                    # Run tests via CTest
make run_tests              # Build and run tests with verbose output
make chip_carving_tests && ./tests/chip_carving_tests  # Direct execution

# Run specific test suites
./tests/chip_carving_tests --gtest_filter="MedialAxisProcessor*"
./tests/chip_carving_tests --gtest_filter="TriArc*"
./tests/chip_carving_tests --gtest_filter="FusionWorkspace*"

# Run specific regression tests (if needed)
# Note: These are already included in 'make test'
./tests/chip_carving_tests --gtest_filter="CoordinateSystemRegressionTest.*"
./tests/chip_carving_tests --gtest_filter="SurfaceZDetectionRegressionTest.*"

# Test Coverage Analysis (requires: brew install lcov)
cmake .. -DENABLE_COVERAGE=ON         # Enable coverage build
make coverage-core                     # Analyze non-Fusion code coverage
make coverage-summary                  # Quick coverage summary
make coverage                         # Full coverage report
# HTML report: open coverage_report/index.html
```

### Code Quality Commands

```bash
# Typical workflow - format then lint
make format lint       # Auto-format and check for issues

# Individual commands
make format            # Auto-format all code with clang-format
make lint              # cpplint style check + file length limits
make lint-quick        # Quick check for critical issues only
make lint-verbose      # Detailed cpplint output with explanations
make lint-tidy         # clang-tidy static analysis (requires: brew install llvm)
make lint-all          # cpplint + clang-tidy combined (comprehensive)
make format-check      # Check if formatting is needed (dry run)

# Pre-commit workflow
make pre-commit        # Run quality checks before committing
```

### Version Management

Follows [semver.org](https://semver.org) with automatic git hash for dev builds:

| VERSION file | Build outputs        | Use case                             |
| ------------ | -------------------- | ------------------------------------ |
| `1.0.1-dev`  | `1.0.1-dev+abc1234`  | Development (git hash auto-appended) |
| `1.0.1-rc.1` | `1.0.1-rc.1+abc1234` | Release candidate                    |
| `1.0.1`      | `1.0.1`              | Release (clean, no hash)             |

The manifest file (`chip_carving_paths_cpp.manifest`) is auto-generated from VERSION via CMake.

### Branching Strategy

Uses Git Flow with two worktrees:

| Worktree | Branch | Purpose |
|----------|--------|---------|
| `carving-fusion` | main | 1.0.x patch releases (bug fixes) |
| `carving-fusion-dev` | develop | 1.1.0+ feature development |

**Keeping branches in sync:**

Periodically merge main into develop to incorporate bug fixes:

```bash
# In carving-fusion-dev worktree
git fetch origin
git merge main
```

This prevents large merge conflicts when releasing feature versions.

**Release workflow (patch release on main):**

1. In `carving-fusion` worktree: change VERSION to `1.0.X` (remove `-dev`)
2. Commit, tag (`git tag v1.0.X`), push with tags
3. Bump VERSION to `1.0.(X+1)-dev`
4. Merge main into develop to sync the fix

**Release workflow (feature release from develop):**

1. Merge main into develop (get any pending fixes)
2. Change VERSION to `1.1.0` (remove `-dev`)
3. Merge develop into main
4. Tag (`git tag v1.1.0`), push main with tags
5. Bump develop VERSION to `1.2.0-dev`

### Complete Make Targets Reference

```bash
# Primary targets
make                   # Build plugin and tests
make install           # Build and install plugin to Fusion 360
make clean             # Clean build artifacts
make test              # Run all tests via CTest

# Test targets
make chip_carving_tests        # Build test executable
make run_tests                # Build and run tests with verbose output
make standalone_medial_axis_test  # Build standalone test program

# Code quality targets
make format            # Auto-format code with clang-format
make format-check      # Check if formatting needed (no changes)
make lint              # Full cpplint style checking
make lint-quick        # Quick cpplint (critical issues only)
make lint-verbose      # Detailed cpplint with explanations
make quality-check     # Run comprehensive quality checks
make pre-commit        # Format and lint before committing

# Version management
make increment_version # Manually increment patch version (legacy)
```

## High-Level Architecture

### Layered Design Pattern

The codebase follows a strict layered architecture with dependency injection:

1. **Plugin Layer** (`src/core/`): Orchestrates the entire workflow
   - `PluginManager`: Main controller, coordinates all operations
   - `ParameterManager`: Handles user inputs and command parameters
   - Mode control via `CHIP_CARVING_PLUGIN_MODE` environment variable

2. **Geometry Layer** (`src/geometry/`): Core computational engine
   - `MedialAxisProcessor`: OpenVoronoi integration for toolpath generation
   - `Shape` hierarchy: Leaf (vesica piscis) and TriArc (curved triangles)
   - Critical coordinate transformations: World ↔ Unit Circle ↔ Voronoi

3. **Adapter Layer** (`src/adapters/`): Fusion 360 API abstraction
   - `FusionAPIAdapter`: Main interface to Fusion API
   - `FusionWorkspace*`: Cross-component geometry extraction
   - Mock implementations for testing without Fusion dependencies

4. **Command Layer** (`src/commands/`): User interaction handling
   - Selection processing and validation
   - Parameter collection and defaults
   - Error handling and user feedback

### Critical Cross-Component Support

The plugin uniquely handles complex Fusion 360 models where geometry exists across multiple components:

- Sketches can be in any component (not just root)
- Surface detection works across all components including mesh bodies
- Proper world coordinate handling throughout

### OpenVoronoi Integration Pipeline

```
Design JSON → Shape Objects → Unit Circle Transform → OpenVoronoi
    → Medial Axis → World Transform → 3D Sketch Paths → CAM Ready
```

Key constraint: OpenVoronoi requires all coordinates within unit circle (radius ≤ 1.0)

## Schema and Constants Management

### Design File Schema

The plugin uses a local copy of `design-schema-v2.json` located in `schema/` directory:

- **Current version**: 2.0
- **Purpose**: Validates JSON design files containing Leaf and TriArc shapes
- **Parser**: `DesignParser` class performs manual JSON parsing (no external JSON library)
- **Validation**: Basic field checking, version verification (must be "2.0")

### Shared Constants

Constants are synchronized across the carving ecosystem:

- **Source**: `schema/constants.json` - master constants file
- **C++ Header**: `include/core/SharedConstants.h` - auto-generated from constants.json
- **Usage**: Geometry calculations, medial axis parameters, rendering settings

### Schema Synchronization

The design schema and constants should match the `@carving/schema` npm package:

- Schema package location: `../carving-schema/`
- Both projects maintain local copies for build independence
- Manual synchronization required when schema updates

## Key Implementation Details

### Shape Processing Rules

- **TriArc negative curvature**: Curves bend INWARD (concave for chip carving)
- **Coordinate precision**: Maintain sub-millimeter accuracy through transforms
- **Bulge factor range**: [-0.2, -0.001] with automatic clamping
- **Shape-by-shape processing**: Individual bounding box and scaling

### V-Carve Depth Calculation

```cpp
depth = clearance_radius * tan(v_bit_angle/2)
final_z = surface_height - depth
```

### File Organization Guidelines

- **350 line limit**: Split files that exceed this
- **Test coverage**: Every geometry calculation needs tests
- **Mock adapters**: No Fusion API calls in unit tests
- **Visual verification**: SVG output for geometry debugging

## Development Workflow

### Adding New Features

1. Write non-fragile unit tests first (TDD approach)
2. Implement in appropriate layer (geometry/adapter/command)
3. Run `make lint-quick` for fast validation
4. Run `make test` to verify functionality
5. Run `make pre-commit` before committing
6. Update CLAUDE.md if architecture changes

### Debugging Issues

1. Enable debug build: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
2. Check logs in `temp_output/logs/`
3. Generate SVG visualizations in `temp_output/svg/`
4. Use extensive console logging in Fusion 360
5. Run specific regression tests if geometry issues

### Performance Profiling

When experiencing slow performance:

1. Add timing code to suspect functions
2. Check OpenVoronoi computation time (usually the bottleneck)
3. Monitor Fusion API call overhead
4. Use macOS Instruments for detailed profiling

## Critical Regression Prevention

### Coordinate System Alignment

**Test**: `test_CoordinateSystemRegression.cpp`

- Prevents medial axis offset from shape boundaries
- Detects world vs local coordinate confusion

### Surface Z Detection

**Test**: `test_SurfaceZDetectionRegression.cpp`

- Ensures V-carve paths on correct surface
- Validates surface height calculations

### Cross-Component Geometry

**Implementation**: `FusionWorkspaceProfile.cpp`, `FusionWorkspaceCurve.cpp`

- Searches ALL components for sketches and surfaces
- Handles mesh bodies (STL/OBJ imports)
- Maintains world coordinate consistency

## Plugin Modes

Control via `CHIP_CARVING_PLUGIN_MODE` environment variable:

- `STANDARD`: Production mode
- `DEBUG`: Enhanced logging
- `COMMANDS_ONLY`: Minimal UI for testing
- `UI_SIMPLE`: Simplified interface

## Quality Standards

### Before Committing

1. Install the plugin with `make install` (catches build issues tests miss)
2. Run `make format lint` (must pass)
3. Run `make test` (all tests must pass)
4. Check for compiler warnings
5. Verify no hardcoded paths or debug code

### Testing Philosophy

- **Non-fragile tests**: Test behavior, not implementation
- **Mock external APIs**: No Fusion 360 dependencies in tests
- **Visual verification**: Generate SVGs for geometry validation
- **Regression prevention**: Dedicated tests for historical bugs

## External Dependencies

- **OpenVoronoi**: Voronoi diagram computation (linked via CMake)
- **Boost**: Required by OpenVoronoi
- **GoogleTest**: Unit testing framework
- **Fusion 360 C++ API**: Plugin host environment

## Important Constraints

1. **Unit circle requirement**: OpenVoronoi needs all coordinates |x|,|y| ≤ 1.0
2. **Negative curvature only**: Chip carving requires concave cuts
3. **Cross-component support**: Must search all components for geometry
4. **350 line file limit**: Refactor larger files
5. **No Fusion API in tests**: Use mock adapters exclusively
6. **C++14 standard**: Code must be compatible with C++14

## Development Tips

- When running lint, typically use `make format lint` to auto-format first
- Before commit and push, install the plugin since it sometimes shows compiler problems that don't show up with test programs

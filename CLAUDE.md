# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

The Fusion 360 CNC Chip Carving Plugin is a mature C++ add-in (v0.9.87) that converts 2D chip carving design shapes into 3D CNC toolpaths using medial axis computation. The plugin generates V-carve toolpaths that preserve traditional chip carving aesthetics while enabling precision CNC fabrication.

## Common Development Commands

### Build and Install
```bash
# Standard build workflow (from project root)
mkdir build && cd build
cmake ..
make                    # Build everything
make test              # Run tests via CTest
make install-fusion    # Install to Fusion 360

# Debug build with full symbols
cmake -DCMAKE_BUILD_TYPE=Debug .. && make

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Clean rebuild
make clean             # Clean build artifacts
# or for complete clean:
rm -rf build && mkdir build && cd build && cmake ..

# Common workflows
make test_and_build    # Run tests, only build if they pass
make run_tests         # Build and run tests with verbose output
make chip_carving_tests && ./tests/chip_carving_tests  # Direct test execution
```

### Testing Commands
```bash
# Run all tests (287+ unit tests)
make test                    # Run tests via CTest
make run_tests              # Run tests with verbose output
make chip_carving_tests && ./tests/chip_carving_tests  # Direct execution

# Run specific test suites
./tests/chip_carving_tests --gtest_filter="MedialAxisProcessor*"
./tests/chip_carving_tests --gtest_filter="TriArc*"
./tests/chip_carving_tests --gtest_filter="FusionWorkspace*"

# Generate code coverage report
cmake .. -DENABLE_COVERAGE=ON    # Configure with coverage
make coverage                    # Run tests and generate report

# Run specific regression tests (if needed)
# Note: These are already included in 'make test'
./tests/chip_carving_tests --gtest_filter="CoordinateSystemRegressionTest.*"
./tests/chip_carving_tests --gtest_filter="SurfaceZDetectionRegressionTest.*"
```

### Code Quality Commands
```bash
# Full lint check
make lint

# Quick lint (critical issues only) 
make lint-quick

# Auto-format code
make format

# Pre-commit checks (format + lint-quick)
make pre-commit

# Run all quality checks before committing
make lint-quick && ./run_tests.sh
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
- **Note**: Header comment says "Auto-generated from interface/constants.json" but actual source is `schema/constants.json`

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
4. Run `./run_tests.sh` to verify functionality
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
- Run: `./run_coordinate_regression_tests.sh`

### Surface Z Detection  
**Test**: `test_SurfaceZDetectionRegression.cpp`
- Ensures V-carve paths on correct surface
- Validates surface height calculations
- Run: `./run_surface_z_regression_tests.sh`

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
1. Run `make lint-quick` (must pass)
2. Run `./run_tests.sh` (all tests must pass)
3. Check for compiler warnings
4. Verify no hardcoded paths or debug code
5. Update VERSION file if significant change

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
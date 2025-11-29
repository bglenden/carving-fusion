# Carving Fusion - C++ Plugin for Autodesk Fusion 360

A native C++ plugin for Autodesk Fusion 360 that converts 2D chip carving designs into precision CNC toolpaths using advanced medial axis computation and V-carve geometry.

**Important**: This file should be read in conjunction with `CLAUDE.md`. Both documents contain important project information. In the event of any conflict between these files, please query the user for clarification.

## Project Overview

**Carving Fusion** bridges traditional chip carving aesthetics with modern CNC fabrication. The plugin imports JSON design files containing Leaf and TriArc chip carving shapes, automatically generates V-carve toolpaths using OpenVoronoi medial axis computation, and creates 3D sketches ready for Fusion 360 CAM workflows.

### Key Features
- **Design Import**: Load JSON design files with Leaf and TriArc shapes (Schema v2.0)
- **Medial Axis Computation**: OpenVoronoi integration for optimal V-carve depth calculation
- **3D Surface Projection**: Map 2D patterns onto curved surfaces and components
- **Cross-Component Support**: Works with complex models spanning multiple components
- **V-Carve Toolpath Generation**: Converts medial axis to CNC-ready toolpaths with depth control
- **CAM Integration**: Generated sketches integrate directly into Fusion 360 CAM workflows

## Technology Stack

### Core Technologies
- **Language**: C++14 (for Fusion 360 API compatibility)
- **Build System**: CMake 3.20+
- **CAD Platform**: Autodesk Fusion 360 C++ API
- **Geometry Engine**: OpenVoronoi library for medial axis computation
- **Testing**: GoogleTest framework
- **Dependencies**: Boost libraries (required by OpenVoronoi)

### External Dependencies
- **OpenVoronoi**: Voronoi diagram and medial axis computation
- **Boost**: C++ libraries (system, required by OpenVoronoi)
- **GoogleTest**: Unit testing framework
- **Autodesk Fusion 360 SDK**: Plugin host environment

## Project Architecture

### Layered Design Pattern
The codebase follows strict layered architecture with dependency injection:

```
Plugin Layer (src/core/)
├── PluginManager: Main controller and workflow orchestration
├── PluginInitializer: Plugin lifecycle management
└── Parameter management and mode control

Geometry Layer (src/geometry/)
├── MedialAxisProcessor: OpenVoronoi integration
├── VCarveCalculator: Toolpath generation and depth calculation
├── Shape hierarchy: Leaf (vesica piscis) and TriArc (curved triangles)
└── Coordinate transformation pipeline

Adapter Layer (src/adapters/)
├── FusionAPIAdapter: Main Fusion 360 API interface
├── FusionWorkspace*: Cross-component geometry extraction
├── FusionSketch: 3D sketch creation and management
└── Mock implementations for testing

Command Layer (src/commands/)
├── User interaction handling
├── Selection processing and validation
├── Parameter collection and defaults
└── Error handling and user feedback

Parser Layer (src/parsers/)
└── DesignParser: JSON design file parsing (manual parsing, no external JSON library)

Utility Layer (src/utils/)
├── Logging system
├── Error handling
├── Component traversal
└── UI parameter helpers
```

### Critical Implementation Details

#### OpenVoronoi Integration Pipeline
```
Design JSON → Shape Objects → Unit Circle Transform → OpenVoronoi
    → Medial Axis → World Transform → V-Carve Paths → 3D Sketch → CAM Ready
```

**Key Constraint**: OpenVoronoi requires all coordinates within unit circle (radius ≤ 1.0)

#### V-Carve Depth Calculation
```cpp
depth = clearance_radius * tan(v_bit_angle/2)
final_z = surface_height - depth
```

#### Cross-Component Support
The plugin uniquely handles complex Fusion 360 models where geometry exists across multiple components:
- Sketches in any component hierarchy (not just root)
- Surface detection works across all components including mesh bodies (STL/OBJ imports)
- Proper world coordinate handling throughout

## Build System

### Build Commands
```bash
# Standard build workflow
mkdir build && cd build
cmake ..
make                    # Build plugin and tests
make install           # Install to Fusion 360

# Debug build with full symbols
cmake -DCMAKE_BUILD_TYPE=Debug .. && make

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Clean rebuild
make clean
# or complete clean:
rm -rf build && mkdir build && cd build && cmake ..
```

### Installation
The plugin installs to Fusion 360 AddIns directory:
- **macOS**: `~/Library/Application Support/Autodesk/Autodesk Fusion 360/API/AddIns/chip_carving_paths_cpp/`
- **Windows**: `%APPDATA%\Autodesk\Autodesk Fusion 360\API\AddIns\chip_carving_paths_cpp/`

## Testing Strategy

### Test Commands
```bash
# Run all tests (287+ unit tests)
make test                    # Run via CTest
make run_tests              # Build and run with verbose output
make chip_carving_tests && ./tests/chip_carving_tests  # Direct execution

# Run specific test suites
./tests/chip_carving_tests --gtest_filter="MedialAxisProcessor*"
./tests/chip_carving_tests --gtest_filter="TriArc*"
./tests/chip_carving_tests --gtest_filter="VCarveCalculator*"

# Test coverage analysis (requires lcov)
cmake .. -DENABLE_COVERAGE=ON
make coverage-core         # Non-Fusion code coverage
make coverage-summary      # Quick coverage summary
make coverage             # Full coverage report
```

### Test Organization
- **Unit Tests**: Core geometry, parsing, calculations (no Fusion API dependencies)
- **Integration Tests**: Full workflow validation
- **Regression Tests**: Historical bug prevention
- **Visual Tests**: SVG generation for geometry validation
- **Mock Adapters**: Test without Fusion 360 dependencies

### Critical Regression Tests
- **Coordinate System Alignment**: Prevents medial axis offset issues
- **Surface Z Detection**: Ensures V-carve paths on correct surface
- **Cross-Component Geometry**: Validates multi-component support

## Code Quality Standards

### Code Style Guidelines
- **C++ Standard**: C++14 (Fusion 360 API requirement)
- **Style**: Google C++ Style Guide with customizations
- **Line Length**: 120 characters maximum
- **File Size**: 350 lines maximum (split larger files)
- **Naming**: CamelCase for classes, lower_case for variables, UPPER_CASE for constants

### Quality Commands
```bash
# Format and lint workflow
make format lint       # Auto-format and check for issues

# Individual commands
make format            # Auto-format with clang-format
make lint              # Full cpplint checking
make lint-quick        # Critical issues only
make pre-commit        # Complete quality check before committing
```

### Static Analysis
- **cpplint**: C++ style and C++14 compatibility checking
- **clang-format**: Automatic code formatting
- **clang-tidy**: Comprehensive static analysis (optional, requires LLVM)
- **File length validation**: Enforced 350-line limit

## Development Workflow

### Adding New Features
1. Write non-fragile unit tests first (TDD approach)
2. Implement in appropriate layer (geometry/adapter/command)
3. Run `make lint-quick` for fast validation
4. Run `make test` to verify functionality
5. Run `make pre-commit` before committing
6. Update documentation if architecture changes

### Plugin Modes
Control via `CHIP_CARVING_PLUGIN_MODE` environment variable:
- `STANDARD`: Production mode
- `DEBUG`: Enhanced logging and visualization
- `COMMANDS_ONLY`: Minimal UI for testing
- `UI_SIMPLE`: Simplified interface

### Debugging
1. Enable debug build: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
2. Check logs in `temp_output/logs/`
3. Generate SVG visualizations in `temp_output/svg/`
4. Use console logging in Fusion 360
5. Run specific regression tests for geometry issues

## Design File Format

### Schema Version 2.0
The plugin reads design files conforming to `design-schema-v2.json`:
- **Leaf shapes**: Vesica piscis (pointed oval) elements defined by two vertices and radius
- **TriArc shapes**: Curved triangles with configurable negative curvature (concave for chip carving)
- **Layout information**: Position, rotation, and scale in world coordinates
- **Validation**: Manual JSON parsing with basic field checking and version verification

### Constants Management
Constants are synchronized across the carving ecosystem:
- **Source**: `schema/constants.json` - master constants file
- **C++ Header**: `include/core/SharedConstants.h` - auto-generated from constants.json
- **Usage**: Geometry calculations, medial axis parameters, rendering settings

## Version Management

### Semantic Versioning
Follows [semver.org](https://semver.org) with automatic git hash for dev builds:

| VERSION file | Build Output | Use Case |
|--------------|-------------|----------|
| `1.0.1-dev` | `1.0.1-dev+abc1234` | Development |
| `1.0.1-rc.1` | `1.0.1-rc.1+abc1234` | Release candidate |
| `1.0.1` | `1.0.1` | Release |

### Release Workflow
1. Development: `VERSION` = `X.Y.Z-dev`
2. Ready to release: change to `X.Y.Z` (remove `-dev`)
3. After release: bump to `X.Y.(Z+1)-dev`

## Important Constraints

1. **Unit Circle Requirement**: OpenVoronoi needs all coordinates |x|,|y| ≤ 1.0
2. **Negative Curvature Only**: Chip carving requires concave cuts (negative bulge values)
3. **Cross-Component Support**: Must search ALL components for geometry
4. **File Size Limit**: 350 lines maximum per file
5. **No Fusion API in Tests**: Use mock adapters exclusively
6. **C++14 Standard**: Code must be compatible with C++14
7. **Manual JSON Parsing**: No external JSON libraries (dependency minimization)

## Security Considerations

- **Input Validation**: All design files validated against schema v2.0
- **Coordinate Bounds**: Enforced unit circle constraints for OpenVoronoi
- **Memory Management**: RAII patterns, no raw pointers in modern code
- **Error Handling**: Comprehensive error propagation to user interface
- **File Path Validation**: Restricted to designated plugin directories

## Performance Optimization

### Known Bottlenecks
- **OpenVoronoi Computation**: Primary performance bottleneck for complex designs
- **Fusion API Calls**: Overhead for geometry creation and modification
- **V-Carve Spline Creation**: NURBS curve processing can cause delays

### Optimization Strategies
- Simplify shapes or reduce pattern density for complex designs
- Batch Fusion API operations where possible
- Use construction geometry for visual preview
- Profile with macOS Instruments for detailed analysis

## Common Issues and Solutions

### Plugin Installation
- Verify plugin folder is in correct AddIns directory
- Check manifest file presence and validity
- Restart Fusion 360 completely after installation

### Design File Import
- Ensure JSON file matches schema version 2.0
- Validate proper shape definitions (Leaf or TriArc types)
- Check coordinate system and units consistency

### Toolpath Generation
- Verify surface selection includes target geometry
- Ensure sketch and surface are in consistent units
- Check surface normal faces correct direction
- Monitor OpenVoronoi computation for complex shapes

This project represents a mature, production-ready C++ plugin with comprehensive testing, strict code quality standards, and sophisticated geometry processing capabilities for professional CNC chip carving workflows.
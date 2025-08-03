# CLAUDE.md - Fusion 360 CNC Chip Carving Plugin

## Project Overview

The Fusion 360 CNC Chip Carving Plugin is a C++ add-in that converts 2D chip carving design shapes into 3D sketch paths suitable for CNC machining. The plugin generates medial axis-based toolpaths that preserve the traditional chip carving aesthetic while enabling precision CNC fabrication.

## Architecture & Tech Stack

### Core Technology
- **Platform**: Autodesk Fusion 360 C++ API
- **Language**: C++ 17 with comprehensive test coverage (154 tests)
- **Build System**: CMake with dependency injection architecture
- **External Library**: OpenVoronoi for medial axis computation
- **Testing**: GoogleTest framework with non-fragile business logic tests

### Key Dependencies
- **Fusion 360 API**: Native C++ integration for sketch creation and CAM preparation
- **OpenVoronoi**: C++ library for Voronoi diagrams and medial axis computation
- **GoogleTest**: Unit testing framework with comprehensive coverage
- **CMake**: Build system with external library linking

## Project Structure

```
fusion_plugin_cpp/
├── CLAUDE.md                  # This documentation file
├── CMakeLists.txt            # Main build configuration with OpenVoronoi linking
├── chip_carving_paths_cpp.manifest  # Fusion 360 plugin manifest
├── include/                  # Header files
│   ├── geometry/            # Shape and geometry processing
│   │   ├── Shape.h          # Base shape interface + utility functions
│   │   ├── Point2D.h        # 2D point utilities and transformations
│   │   ├── Leaf.h           # Vesica piscis (fish-eye) shaped cuts
│   │   ├── TriArc.h         # Triangular cuts with curved edges
│   │   ├── ShapeFactory.h   # JSON deserialization to shape objects
│   │   ├── SVGGenerator.h   # Visual verification via SVG output
│   │   └── MedialAxisProcessor.h  # OpenVoronoi integration (MVP Phase 2)
│   ├── parsers/            # Design file processing
│   │   └── DesignParser.h  # JSON schema validation and parsing
│   └── core/               # Plugin management
│       └── PluginManager.h # Command execution and lifecycle
├── src/                    # Implementation files
│   ├── geometry/           # Core geometric algorithms
│   ├── adapters/           # Fusion 360 API abstraction layer
│   ├── commands/           # Plugin command implementations
│   ├── core/               # Plugin lifecycle management
│   └── parsers/            # JSON processing implementation
├── tests/                  # Comprehensive test suite (154 tests)
│   ├── geometry/           # Shape and utility tests
│   ├── adapters/           # Mock adapter tests (no Fusion API dependencies)
│   ├── core/               # Plugin manager tests
│   └── parsers/            # JSON parsing validation tests
├── resources/              # Plugin icons and UI assets
```

## MVP Goals and Scope

### What This Plugin DOES (MVP Scope)
1. **Import 2D Designs**: Load JSON design files with Leaf and TriArc shapes
2. **Generate Medial Axis**: Compute centerline paths using OpenVoronoi with clearance radii
3. **Create 3D Sketch Paths**: Generate spline curves along medial axis with appropriate Z-depths
4. **Visualize Construction Geometry**: Display medial lines and clearance circles for inspection
5. **Prepare for CAM**: Output 3D sketch curves ready for Fusion 360 CAM toolpath generation

### What This Plugin does NOT do (Out of Scope)
- ❌ Direct G-code generation
- ❌ Complete CAM toolpath creation
- ❌ Tool collision detection
- ❌ Multi-pass depth strategies
- ❌ Cutting speed/feed optimization

The plugin creates **3D sketch paths** that are then used by Fusion 360's native CAM system for final toolpath generation.

## CNC Chip Carving Workflow

### Traditional Chip Carving Process
1. **Design**: Create geometric patterns (typically triangular cuts)
2. **Layout**: Transfer patterns to wood surface
3. **Cutting**: Use chip carving knives to make precise angled cuts
4. **Finishing**: Create clean, faceted surfaces with light/shadow interplay

### CNC Adaptation Strategy
1. **Digital Design**: Create 2D patterns in design program (TypeScript app)
2. **Shape Import**: Load JSON design files into Fusion 360 plugin
3. **Medial Axis Computation**: Calculate centerline paths that preserve cut geometry
4. **3D Path Generation**: Create sketch curves with depth information for V-carve operations
5. **CAM Integration**: Use Fusion 360's native CAM for final toolpath generation

## OpenVoronoi Integration Architecture

### Core Concepts
- **Voronoi Diagram**: Mathematical structure for computing equidistant boundaries
- **Medial Axis**: Skeleton of a shape - the centerline for optimal tool movement
- **Clearance Radius**: Maximum tool size that fits at each medial axis point
- **Unit Circle Constraint**: OpenVoronoi requires all coordinates within radius ≤ 1.0

### Coordinate Transformation Pipeline
```
World Coordinates (mm) → Unit Circle → OpenVoronoi → Unit Circle → World Coordinates
     ↓                      ↓             ↓             ↓              ↓
Design shapes          Scale/translate   Compute      Inverse       3D sketch
(arbitrary position)   to unit circle   medial axis  transform     paths
```

**Critical Implementation Requirements:**
1. **Shape-by-Shape Processing**: Each shape requires individual transformation parameters
2. **Bounding Box Calculation**: Find min/max extents of shape vertices
3. **Center Offset**: Translation vector to center shape at origin
4. **Scale Factor**: Uniform scaling to fit largest dimension within unit circle
5. **Precision Preservation**: Maintain accuracy during bidirectional transforms

### Medial Axis Filtering
- **medial_axis_filter**: OpenVoronoi class for filtering Voronoi edges
- **Threshold Parameter**: Default 0.8, configurable (controls edge parallelism tolerance)
- **polygon_interior_filter**: Removes exterior clearance discs (recommended for CNC)
- **MedialAxisWalk**: Extracts ordered chains of MedialPoint objects

### Output Format
```cpp
struct MedialPoint {
    Point p;                 // (x, y) coordinates in world space
    double clearance_radius; // Maximum tool radius at this point
};
typedef std::list<MedialPoint> MedialPointList;        // Single chain
typedef std::list<MedialPointList> MedialChain;       // Multiple chains
```

## Shape System Architecture

### Base Shape Interface
```cpp
class Shape {
public:
    virtual std::vector<Point2D> getVertices() const = 0;
    virtual void drawToSketch(ISketch* sketch, ILogger* logger) const = 0;
    virtual void getBounds(Point2D& min, Point2D& max) const = 0;
    virtual bool contains(const Point2D& point) const = 0;
    virtual Point2D getCentroid() const = 0;
};
```

### Leaf Shape (Vesica Piscis)
- **Geometry**: Two intersecting circles forming fish-eye shape
- **Parameters**: Two focus points + radius
- **CNC Application**: Creates pointed oval cuts typical in chip carving
- **Arc Calculation**: Precise arc parameters for smooth tool movement

### TriArc Shape (Curved Triangle)
- **Geometry**: Triangle with curved edges controlled by bulge factors
- **Parameters**: Three vertices + three curvature values
- **Constraint**: Bulge factors must be negative (concave arcs only)
- **Range**: [-0.2, -0.001] with automatic clamping
- **CNC Application**: Creates traditional triangular chip cuts with smooth curves

### Critical Geometry Rules (DO NOT FORGET)
**For TriArc shapes with curved edges:**

**Negative curvature (concave arcs)**:
- Curve bends INWARD toward triangle center
- Center of curvature is positioned AWAY from triangle center
- Arc sweeps to bring arc midpoint CLOSER to triangle centroid

**Positive curvature (convex arcs)**:
- Curve bends OUTWARD away from triangle center
- Center of curvature is positioned TOWARD triangle center
- Arc sweeps to bring arc midpoint FARTHER from triangle centroid

**Implementation Algorithm:**
1. Calculate both possible arc centers (perpendicular to chord)
2. For negative curvature: choose center FARTHER from triangle centroid
3. For positive curvature: choose center CLOSER to triangle centroid
4. Choose sweep direction based on geometric calculation, NOT shape definition ordering

## Construction Geometry Visualization

### Medial Axis Display
- **Line Style**: Construction lines (dashed, thin)
- **Color Coding**: 
  - Blue: Primary medial axis chains
  - Gray: Secondary/filtered chains
  - Red: Problem areas (if any)

### Clearance Circle Display
- **Circle Style**: Construction circles (center point + radius)
- **Spacing**: Every Nth point along medial axis (configurable)
- **Purpose**: Visual verification of tool clearance at each position
- **Size Indication**: Circle radius = maximum tool radius at that point

### Sketch Organization
- **Primary Sketch**: Shape outlines for reference
- **Construction Sketch**: Medial lines + clearance circles
- **Toolpath Sketch**: Final 3D spline curves
- **Layer Management**: Different colors/styles for clear inspection

## 3D Sketch Path Generation

### Path Calculation Strategy
1. **Medial Axis Input**: MedialPoint chains with (x, y, clearance_radius)
2. **Depth Calculation**: Use clearance radius + V-bit geometry for Z-depth
3. **Spline Generation**: Create smooth 3D curves connecting medial points
4. **Tool Compensation**: Account for V-bit angle and desired cut depth

### V-Bit Tool Integration
- **Angle Parameter**: Typical values 60°, 90°, 120°
- **Depth Formula**: `depth = clearance_radius * tan(angle/2)`
- **Maximum Depth**: Configurable limit for tool safety
- **Minimum Radius**: Prevent tool from going too deep in tight areas

### Fusion 360 Integration
- **Sketch Curves**: Use Fusion's native spline creation
- **3D Positioning**: Set Z-coordinates based on calculated depths
- **CAM Compatibility**: Ensure curves are suitable for CAM toolpath generation
- **Reference Geometry**: Maintain links to original 2D shapes

## Testing Strategy

### Current Test Coverage (158+ Tests)
- **Geometry Tests**: Shape creation, arc calculations, transformations
- **Parser Tests**: JSON validation, error handling, schema compliance
- **Factory Tests**: Shape creation from JSON, parameter validation
- **SVG Tests**: Visual verification output, coordinate transformations
- **Utility Tests**: Bounding box, centroid calculations, helper functions
- **Coordinate System Regression Tests**: Critical tests preventing medial axis misalignment

### MVP Testing Additions (Planned)
- **MedialAxisProcessor Tests**: Coordinate transformations, OpenVoronoi integration
- **Construction Geometry Tests**: Sketch creation, visualization accuracy
- **Path Generation Tests**: 3D curve creation, depth calculations
- **Integration Tests**: End-to-end workflow validation

### Non-Fragile Testing Philosophy
- **Business Logic Focus**: Test actual computation, not implementation details
- **Mock External APIs**: Avoid fragile dependencies on Fusion 360 API
- **Input/Output Testing**: Verify correct results for given inputs
- **Error Handling**: Test graceful failure modes and error recovery

### Critical Regression Prevention

**Coordinate System Regression Tests** (`test_CoordinateSystemRegression.cpp`):
- **Purpose**: Prevents critical bugs where medial axis appears offset/rotated from shape boundaries
- **Root Cause Protected**: Local vs. world coordinate confusion in `extractProfileVertices()`
- **Test Coverage**:
  - Rectangle medial axis alignment (detects position offset)
  - Scale consistency across different sizes (detects scaling errors)
  - Origin independence (detects coordinate transformation bugs)
  - Boundary constraints (detects medial axis projection errors)
- **Runner Script**: `./run_coordinate_regression_tests.sh`
- **Historical Bug**: When `getCurveWorldGeometry()` returned local coordinates but medial axis processor expected world coordinates, causing 15mm+ positional errors

**Surface Z Detection Regression Tests** (`test_SurfaceZDetectionRegression.cpp`):
- **Purpose**: Prevents critical bugs where V-carve paths appear on wrong surface (top vs bottom)
- **Root Cause Protected**: Parameter space iteration finding closest point on wrong surface
- **Test Coverage**:
  - Surface Z range validation for dome geometries (25-50mm expected range)
  - V-carve depth calculation correctness (surface_height - carve_depth)
  - Coordinate system consistency (mm ↔ cm conversions)
  - Top surface selection logic (ray casting vs parameter space behavior)
  - Edge case handling (NaN, invalid results, extreme coordinates)
- **Runner Script**: `./run_surface_z_regression_tests.sh`
- **Historical Bug**: When parameter space iteration returned bottom surface Z values (-115mm) instead of top surface (25-50mm), causing V-carve paths to appear below geometry instead of carved into top surface

**Enhanced Cross-Component Surface Detection** (`FusionWorkspaceCurve.cpp`):
- **Problem Solved**: Zero height detection when sketch is in root component but surface is in separate component
- **Root Cause**: Original code only searched `rootComp->findBRepUsingRay()`, missing surfaces in sub-components
- **Enhanced Solution**: Universal ray casting across ALL components with mesh support
- **Key Improvements**:
  - **Cross-Component Search**: Recursively searches all component occurrences using `rootComp->allOccurrences()`
  - **Universal Surface Support**: Works with B-Rep bodies AND mesh bodies (STL/OBJ imports)
  - **Coordinate Consistency**: Proper world coordinate handling across different component transforms
  - **Ray Casting Strategy**: Shoots ray from above (1000cm) downward, finds topmost intersection across all components
- **Implementation**:
  ```cpp
  // Search ALL components, not just root
  std::vector<Ptr<Component>> allComponents;
  allComponents.push_back(rootComp);
  auto occurrences = rootComp->allOccurrences();
  for (size_t i = 0; i < occurrences->count(); ++i) {
      allComponents.push_back(occurrences->item(i)->component());
  }
  
  // Ray cast in each component
  for (auto component : allComponents) {
      // B-Rep surface detection
      auto intersectedEntities = component->findBRepUsingRay(
          rayOrigin, rayDirection, BRepFaceEntityType, 0.001, false, hitPoints);
      
      // Mesh surface detection (for imported STL/OBJ)
      auto meshBodies = component->meshBodies();
      // Custom triangle-ray intersection for mesh surfaces
  }
  ```
- **Mesh Support**: Custom ray-triangle intersection for mesh bodies (STL/OBJ files)
- **Performance**: Scales to multiple components while maintaining single-ray efficiency
- **Robustness**: Handles edge cases like empty components, coordinate transform failures
- **Debug Logging**: Comprehensive logging shows component search progress and intersection results

**Enhanced Cross-Component Geometry Extraction** (`FusionWorkspaceProfile.cpp`):
- **Problem Solved**: "Failed to extract geometry from selected profiles" when closed path geometry is in a component sketch (not root)
- **Root Cause**: Original code only searched `rootComp->sketches()`, missing sketch profiles in sub-components
- **Enhanced Solution**: Universal sketch search across ALL components with comprehensive logging
- **Key Improvements**:
  - **Cross-Component Sketch Search**: Recursively searches all component occurrences using `rootComp->allOccurrences()`
  - **Profile Entity Token Matching**: Searches all sketches in all components for exact entity token matches
  - **Comprehensive Logging**: Shows components searched, sketches found, and profile matching results
  - **Coordinate Consistency**: Maintains proper world coordinate handling from sketch profiles in any component
- **Implementation**:
  ```cpp
  // Search ALL components for sketches, not just root
  std::vector<Ptr<Component>> allComponents;
  allComponents.push_back(rootComp);
  auto occurrences = rootComp->allOccurrences();
  for (size_t i = 0; i < occurrences->count(); ++i) {
      allComponents.push_back(occurrences->item(i)->component());
  }
  
  // Search each component's sketches for profile entity token
  for (auto component : allComponents) {
      auto sketches = component->sketches();
      // ... search all profiles in all sketches
  }
  ```
- **Compatibility**: Works with existing geometry extraction pipeline while extending cross-component support
- **Performance**: Efficient component traversal with early termination when profile found
- **Debug Output**: Detailed logging shows exactly which component contained the selected profile

## Development Build Configuration

### Build Types: Debug vs Release

The plugin supports two build configurations with different optimization levels and version labeling:

#### Release Build (Production)
**Use for**: Production deployment, performance testing, final builds
```bash
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make && make install
```
- **Optimized**: Full compiler optimizations (-O3, -DNDEBUG)
- **Version**: Shows `v0.3.xxx-release` in console
- **Performance**: Fastest execution, minimal logging
- **Use Case**: Final deployment, performance testing

#### Debug Build (Development)  
**Use for**: Development, debugging, troubleshooting issues
```bash
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make && make install
```
- **Debug Symbols**: Full debugging information (-g3, -O0)
- **Version**: Shows `v0.3.xxx-debug` in console  
- **Assertions**: All debug checks enabled
- **Use Case**: Development, debugging, crash investigation

### Version String Behavior
- **Dynamic Generation**: Version suffix automatically reflects build type
- **Auto-Increment**: Patch version increments on each build (0.3.205 → 0.3.206)
- **Console Display**: Fusion 360 console shows actual build type in startup message
- **Template System**: CMake generates `src/version.h` from `src/version.h.in`
```

### Performance Profiling

When the plugin feels slow, use these profiling approaches to identify bottlenecks:

#### Option 1: Built-in Timing (Easiest)
Add timing code to key functions in your workflow:
```cpp
#include <chrono>

// At start of function
auto start = std::chrono::high_resolution_clock::now();

// Your code here

// At end of function  
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
logger_->logInfo("Function took: " + std::to_string(duration.count()) + "ms");
```

**Key areas to profile:**
- `PluginManager::importDesign()` - JSON parsing and shape creation
- `MedialAxisProcessor::computeMedialAxis()` - OpenVoronoi computation  
- `TriArc::drawToSketch()` - Fusion API sketch creation
- `Leaf::drawToSketch()` - Arc drawing operations

#### Option 2: Instruments (macOS Native)
```bash
# Build with debug symbols for profiling
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make && make install

# Launch Fusion 360 through Instruments
instruments -t "Time Profiler" /Applications/Autodesk\ Fusion\ 360/Fusion\ 360.app
# Then trigger your plugin operations
```

#### Option 3: Console Timing Analysis
Enable detailed logging and measure operations:
```bash
# 1. Build with timing logging enabled
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TIMING_LOGS=ON ..

# 2. Run operations and check Fusion console for timing info
# Look for patterns like:
# "MedialAxis computation: 2847ms" 
# "Shape creation: 12ms"
# "Sketch drawing: 156ms"
```

#### Option 4: Sampling with Activity Monitor
1. Open Activity Monitor → CPU tab
2. Find "Fusion 360" process 
3. Double-click → Sample Process (during slow operations)
4. Look for hotspots in call stack

#### Common Performance Bottlenecks
Based on the codebase, likely slow areas:
- **OpenVoronoi computation**: Most CPU-intensive, especially for complex shapes
- **Fusion API calls**: Each `sketch->addArcByThreePoints()` has overhead
- **File I/O**: Large JSON design file parsing
- **Memory allocation**: Frequent vector/list operations in medial axis processing

#### Quick Performance Check
```bash
# Time a typical workflow
time fusion360_cli_operation_if_available
# Or measure in Fusion console timestamps
```

## User Interface Integration

### Plugin Commands
1. **Import Design**: Load JSON file with shapes
2. **Generate Medial Axis**: Compute centerlines with OpenVoronoi
3. **Create Construction Geometry**: Visualize medial lines + clearance circles
4. **Generate 3D Paths**: Create sketch curves for CAM
5. **Configure Parameters**: Set medial axis threshold, tool parameters

### Parameter Configuration
- **Medial Axis Threshold**: 0.8 default (edge parallelism tolerance)
- **V-Bit Angle**: 60°, 90°, 120° options
- **V-Bit Diameter**: Tool size for depth calculations
- **Maximum Cut Depth**: Safety limit for tool penetration
- **Visualization Options**: Show/hide construction geometry

## Error Handling and Diagnostics

### Common Issues and Solutions
1. **Coordinate Overflow**: Ensure shapes fit within unit circle after scaling
2. **Degenerate Geometry**: Validate triangle areas > minimum threshold
3. **OpenVoronoi Failures**: Check input geometry for validity
4. **Memory Issues**: Handle large datasets gracefully
5. **Precision Loss**: Maintain accuracy during coordinate transformations

### Diagnostic Tools
- **SVG Generation**: Visual verification of shape processing
- **Debug Logging**: Detailed output during medial axis computation
- **Construction Geometry**: Visual inspection of computed results
- **Test Suite**: Comprehensive validation of all components

## Future Enhancement Opportunities

### Phase 2 Features (Beyond MVP)
- **Multi-Tool Support**: Different tools for different cut types
- **Advanced Filtering**: Custom medial axis filtering strategies
- **Optimization**: Tool path optimization for reduced machining time
- **Material Integration**: Wood grain direction considerations
- **Advanced Visualization**: 3D preview of final carved results

### Integration Possibilities
- **Design Program Link**: Direct connection to TypeScript design application
- **CAM Automation**: Automatic CAM setup with optimized parameters
- **Post-Processing**: Custom post-processors for specific CNC machines
- **Quality Control**: Automatic verification of generated toolpaths

## Contributing Guidelines

### Code Standards
- **File Size Limit**: 350 lines maximum - refactor when exceeded
- **Test Coverage**: Non-fragile tests for all business logic
- **Documentation**: Update CLAUDE.md for significant changes
- **Commit Strategy**: Small, logical chunks with working tests

### Build Requirements
- **C++17 Standard**: Modern C++ features and library support
- **CMake 3.20+**: For proper dependency management
- **Fusion 360 API**: Current version with C++ support
- **OpenVoronoi**: Debug build with all assertions enabled

This documentation serves as the definitive guide for understanding, developing, and extending the CNC Chip Carving Fusion 360 plugin.

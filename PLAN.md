# Fusion Plugin Development Plan

> **Note:** This plan covers features specific to the C++ Fusion 360 plugin.
> See [Master Plan](../PLAN.md) for overall project roadmap.

## Current Status

### Implemented Features ✅
- C++ plugin architecture with CMake build
- Fusion API abstraction layer (dependency injection)
- JSON design import (Schema v2.0)
- Shape parsing: Leaf and Tri-Arc
- OpenVoronoi integration
- Medial axis computation from profiles
- Enhanced UI for profile selection
- Construction geometry visualization
- Comprehensive logging system
- GoogleTest unit test suite

### Current Limitations ❌
- No actual toolpath generation (only medial axis)
- No depth control or 3D operations
- No tool selection interface
- No CAM operation creation
- Limited to 2 shape types
- No curvature-based optimization

## Known Issues

### High Priority
- **No Toolpath Output**: Medial axis doesn't translate to CNC paths
- **No V-Bit Support**: Cannot specify tool geometry
- **No 3D Projection**: Only works on flat sketches

### Medium Priority
- **Limited Shape Support**: Only Leaf and Tri-Arc
- **No Parameter Persistence**: Settings reset each use
- **No Batch Processing**: One profile at a time

### Low Priority
- **No Progress Feedback**: Long operations appear frozen
- **Debug Files Accumulate**: No automatic cleanup
- **V-Carve Spline Performance**: NURBS curve processing in Fusion 360 causes >1 minute delays during spline creation (identified via Activity Monitor profiling). Root cause: `AcGeImpNurbCurve3d::getClosestPointTo` operations during `addSpline3D()` calls. Potential optimizations: reduce spline complexity, use simpler geometry, batch operations, or disable real-time validation.

## Phase 1: Core Toolpath Generation (Current Priority)

### 1.1 V-Bit Toolpath Generation
**Convert Medial Axis to Toolpaths**:
- [ ] Create `ToolpathGenerator` class
- [ ] Implement V-bit geometry calculations:
  - [ ] Tool angle parameter (30°, 45°, 60°, 90°)
  - [ ] Depth from clearance radius
  - [ ] Contact point calculation
- [ ] Generate toolpath segments:
  - [ ] Convert medial axis chains to tool positions
  - [ ] Add Z-depth based on clearance
  - [ ] Optimize for minimal lift moves
- [ ] Entry/exit strategies:
  - [ ] Plunge entry points
  - [ ] Ramp entry option
  - [ ] Lead-in/lead-out arcs

### 1.2 CAM Operation Integration
**Create Fusion CAM Operations**:
- [ ] Research Fusion CAM API
- [ ] Create setup operations:
  - [ ] Stock definition
  - [ ] Coordinate system
  - [ ] Tool selection
- [ ] Generate toolpath operations:
  - [ ] Create custom toolpath
  - [ ] Set feeds and speeds
  - [ ] Configure coolant
- [ ] Post-processor compatibility:
  - [ ] Ensure standard post support
  - [ ] Test with common machines

### 1.3 Tool Management
**Tool Selection UI**:
- [ ] Create tool selection dialog
- [ ] V-bit angle selection
- [ ] Tool database integration:
  - [ ] Load from Fusion tool library
  - [ ] Custom tool creation
  - [ ] Save tool presets
- [ ] Feed/speed recommendations:
  - [ ] Material-based defaults
  - [ ] Override capabilities

### 1.4 3D Surface Support
**Project onto 3D Surfaces**:
- [ ] Surface selection UI
- [ ] Normal vector calculation
- [ ] Depth adjustment algorithm:
  - [ ] Project 2D pattern
  - [ ] Adjust for surface curvature
  - [ ] Maintain V-bit angles
- [ ] Preview visualization:
  - [ ] Show projected pattern
  - [ ] Display depth variations

## Phase 2: Advanced Manufacturing

### 2.1 Multi-Tool Support
**Different Tool Types**:
- [ ] Ball nose mills:
  - [ ] Rounded cut profiles
  - [ ] Smooth transitions
- [ ] Flat end mills:
  - [ ] Pocket clearing
  - [ ] Flat-bottom cuts
- [ ] Tapered mills:
  - [ ] Variable angle cuts
  - [ ] Draft angles

**Tool Change Optimization**:
- [ ] Group operations by tool
- [ ] Minimize tool changes
- [ ] Tool change positions
- [ ] Time estimation

### 2.2 Advanced Strategies
**Adaptive Clearing**:
- [ ] Detect pocket areas
- [ ] Adaptive toolpaths
- [ ] Chip load optimization
- [ ] Heat management

**Rest Machining**:
- [ ] Previous tool tracking
- [ ] Remaining material detection
- [ ] Pencil cleanup paths
- [ ] Corner optimization

**High-Speed Paths**:
- [ ] Smooth arc fitting
- [ ] Constant engagement
- [ ] Look-ahead optimization
- [ ] Acceleration limits

### 2.3 Material Database
**Wood Properties**:
- [ ] Common species data:
  - [ ] Hardness ratings
  - [ ] Grain considerations
  - [ ] Chip characteristics
- [ ] Cutting parameters:
  - [ ] Recommended speeds
  - [ ] Feed rates
  - [ ] Depth of cut
- [ ] Surface finish optimization:
  - [ ] Climb vs conventional
  - [ ] Final pass parameters

## Phase 3: Professional Features

### 3.1 Simulation and Verification
- [ ] Toolpath simulation
- [ ] Collision detection
- [ ] Material removal visualization
- [ ] Time estimation
- [ ] Surface finish preview

### 3.2 Optimization Engine
- [ ] Toolpath smoothing
- [ ] Feed rate optimization
- [ ] Minimal retract moves
- [ ] Path ordering
- [ ] Trochoidal milling

### 3.3 Integration Features
- [ ] Batch processing
- [ ] Parameter templates
- [ ] Job sheets
- [ ] Setup documentation
- [ ] Tool lists

## Testing Strategy

### Unit Tests
- Toolpath generation algorithms
- Depth calculations
- Tool geometry math
- CAM operation creation

### Integration Tests
- Full workflow tests
- CAM operation validity
- Post-processor output
- Performance benchmarks

### Real-World Testing
- Test cuts in various materials
- Surface finish evaluation
- Accuracy measurements
- Tool wear analysis

## Next Immediate Tasks

1. [ ] Research Fusion CAM API documentation
2. [ ] Implement basic V-bit depth calculation
3. [ ] Create simple toolpath from medial axis
4. [ ] Generate first CAM operation
5. [ ] Test with actual CNC cut

## Success Metrics

- [ ] Generate valid G-code for 3+ CNC machines
- [ ] Toolpath accuracy within 0.1mm
- [ ] Surface finish comparable to commercial software
- [ ] Processing time < 30 seconds for typical design
- [ ] Zero crashes or data loss
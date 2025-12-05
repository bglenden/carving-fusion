# Carving Fusion

A native C++ plugin for Autodesk Fusion 360 that converts 2D chip carving designs into precision CNC toolpaths.

## Overview

Carving Fusion bridges traditional chip carving aesthetics with modern CNC fabrication. Import JSON design files created with Carving Designer, and the plugin automatically generates V-carve toolpaths using advanced medial axis computation. The result: authentic chip carving patterns machined with CNC precision.

## Features

- **Design Import**: Load JSON design files containing Leaf and TriArc chip carving shapes
- **Automatic Toolpath Generation**: Medial axis computation calculates optimal V-carve depths
- **3D Surface Projection**: Map 2D patterns onto curved surfaces and components
- **Cross-Component Support**: Works with complex models where geometry spans multiple components
- **Visual Preview**: See toolpaths in Fusion 360 before machining
- **CAM Integration**: Generated sketches integrate directly into Fusion 360 CAM workflows

## Installation

### Prerequisites

- Autodesk Fusion 360 (current version)
- macOS or Windows (plugin is built for each platform)

### Installing the Plugin

1. Download the latest release from the releases page
2. Extract the plugin files
3. Copy the `chip_carving_paths_cpp` folder to your Fusion 360 AddIns directory:
   - **macOS**: `~/Library/Application Support/Autodesk/Autodesk Fusion 360/API/AddIns/`
   - **Windows**: `%APPDATA%\Autodesk\Autodesk Fusion 360\API\AddIns\`
4. Restart Fusion 360
5. Enable the plugin in **Tools → Add-Ins → Scripts and Add-Ins**

## Usage

1. Create a design using [Carving Designer](https://github.com/bglenden/carving-designer)
2. Export the design as a JSON file
3. In Fusion 360, run the Chip Carving plugin from the **Tools** menu
4. Select a sketch profile or curve for pattern placement
5. Select a surface for projection (or choose XY plane)
6. Load your design JSON file
7. Configure V-bit parameters (angle, clearance)
8. Generate toolpaths

The plugin creates 3D sketches that can be used directly in Fusion 360's CAM workspace for generating CNC programs.

## Building from Source

### Prerequisites

- CMake 3.20 or higher
- C++14 compatible compiler (Xcode on macOS, Visual Studio on Windows)
- Autodesk Fusion 360 SDK
- OpenVoronoi library (for medial axis computation)
- Boost libraries (required by OpenVoronoi)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/bglenden/carving-fusion.git
cd carving-fusion

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Install to Fusion 360
make install
```

For detailed development instructions, see [CLAUDE.md](CLAUDE.md).

## Design File Format

Carving Fusion reads design files conforming to `design-schema-v2.json`. Designs contain:

- **Leaf shapes**: Vesica piscis (pointed oval) elements
- **TriArc shapes**: Curved triangles with configurable bulge
- **Layout information**: Position, rotation, and scale

The schema is shared with the Carving Designer application to ensure compatibility.

## Technical Details

### Coordinate System Assumptions

The plugin currently assumes a specific coordinate orientation:

- **Cutting axis**: Z axis, with smaller Z values representing deeper cuts into the material
- **Design plane**: Sketches must be in a plane parallel to the XY plane (i.e., the plane's normal must be parallel to the Z axis)

This means the workpiece surface should be oriented with its "top" facing the positive Z direction. When selecting a sketch plane or construction plane for your design, ensure it is horizontal (parallel to XY).

### Medial Axis Computation

The plugin uses OpenVoronoi to compute the medial axis (centerline) of each carving shape. This determines the maximum V-bit clearance at each point, which translates directly to carving depth. The algorithm:

1. Transforms shape coordinates to unit circle space (OpenVoronoi requirement)
2. Computes Voronoi diagram and extracts medial axis
3. Calculates clearance radius at each medial axis point
4. Converts clearance to depth: `depth = clearance * tan(v_bit_angle/2)`
5. Transforms back to world coordinates and projects onto surface

### Cross-Component Support

Unlike many Fusion 360 plugins, Carving Fusion can work with geometry distributed across multiple components:

- Sketches in any component hierarchy
- Surfaces from mesh bodies (STL/OBJ imports)
- Proper world coordinate transformation throughout

## Project Structure

```
src/
├── core/           # Plugin orchestration and lifecycle
├── geometry/       # Medial axis and shape processing
├── parsers/        # JSON design file parsing
├── adapters/       # Fusion 360 API abstraction
├── commands/       # User interface and parameter handling
└── utils/          # Logging, error handling, helpers

include/           # Public headers
tests/             # Unit tests (GoogleTest)
resources/         # Plugin manifest and icons
schema/            # Design file schema and constants
```

## Related Projects

- **[carving-schema](https://github.com/bglenden/carving-schema)** - Shared schema and type definitions
- **[carving-designer](https://github.com/bglenden/carving-designer)** - Web-based design application

## Troubleshooting

### Plugin doesn't appear in Fusion 360

- Verify the plugin folder is in the correct AddIns directory
- Check that the manifest file is present
- Restart Fusion 360 completely

### Import fails with "Invalid design file"

- Ensure the JSON file matches schema version 2.0
- Validate the design file structure
- Check for proper shape definitions (Leaf or TriArc types)

### Toolpaths appear offset or incorrect

- Verify surface selection includes the target geometry
- Check that sketch and surface are in consistent units
- Ensure surface normal faces the correct direction

### Performance issues with complex designs

- OpenVoronoi computation is the primary bottleneck
- Simplify shapes or reduce pattern density
- Consider breaking large designs into smaller sections

## Contributing

Contributions are welcome! Please:

1. Write unit tests for new features
2. Follow the existing code style (use `make format lint`)
3. Ensure all tests pass (`make test`)
4. Update documentation as needed

See [CLAUDE.md](CLAUDE.md) for detailed development guidelines.

## License

MIT License - see LICENSE file for details

## Version

This project uses semantic versioning (MAJOR.MINOR.PATCH). See the VERSION file for the current version.

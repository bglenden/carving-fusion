# Carving Fusion

Autodesk Fusion 360 plugin for generating CNC toolpaths from carving designs.

## Overview

Carving Fusion is a native C++ plugin for Autodesk Fusion 360 that converts 2D carving designs into CNC toolpaths. It uses medial axis transformation to compute V-carve depths and can project patterns onto 3D surfaces.

## Features

- **Design Import**: Load JSON design files created with Carving Designer
- **Medial Axis Computation**: Calculate centerlines and clearance for V-carve operations
- **3D Projection**: Map 2D patterns onto curved surfaces
- **Toolpath Generation**: Create optimized CNC engraving operations
- **Visual Verification**: Preview toolpaths before machining

## Building

### Prerequisites

- CMake 3.20 or higher
- C++17 compatible compiler
- Autodesk Fusion 360 SDK
- OpenVoronoi library (for medial axis computation)

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

### Installation

```bash
make install
```

This will install the plugin to the Fusion 360 plugins directory.

## Testing

```bash
cd build
ctest
```

The project uses GoogleTest for unit testing.

## Project Structure

```
src/
├── core/           # Core algorithms and data structures
├── geometry/       # Geometric computations
├── parsers/        # JSON design file parsing
├── fusion/         # Fusion 360 API integration
└── ui/            # Plugin UI components

include/           # Header files
tests/            # Unit tests
resources/        # Plugin manifest and icons
schema/           # Design file schema
```

## Dependencies

- **OpenVoronoi**: Used for computing medial axes and Voronoi diagrams
- **nlohmann/json**: JSON parsing library
- **GoogleTest**: Testing framework

## Schema

The plugin reads design files conforming to `design-schema-v2.json`. The schema is vendored in the `schema/` directory to ensure compatibility.

## Related Projects

- [carving-schema](https://github.com/bglenden/carving-schema) - Shared schema and type definitions
- [carving-designer](https://github.com/bglenden/carving-designer) - Web-based design application

## Development

### Code Style

The project uses clang-format for C++ code formatting. Configuration is in `.clang-format`.

### Running Lint

```bash
make lint
```

### Incremental Version Updates

The build system automatically increments the plugin version on each build.

## License

MIT
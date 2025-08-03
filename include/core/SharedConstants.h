/**
 * Shared constants for CNC Chip Carving
 * Auto-generated from interface/constants.json - DO NOT EDIT MANUALLY
 */

#pragma once

namespace ChipCarving {
namespace Constants {

// Factor for calculating default leaf radius from vertex distance
namespace Leaf {
constexpr double DEFAULT_RADIUS_FACTOR = 0.65;
}

// TriArc curvature parameters for concave arcs
namespace Triarc {
constexpr double DEFAULT_BULGE = -0.125;
constexpr double BULGE_RANGE_MIN = -0.2;
constexpr double BULGE_RANGE_MAX = -0.001;
}  // namespace Triarc

// Floating point comparison tolerance for geometric calculations
namespace Epsilon {
constexpr double TOLERANCE = 1e-9;
}

// Valid range for arc sagitta as ratio of chord length
// OpenVoronoi medial axis computation parameters
namespace MedialAxis {
constexpr double DEFAULT_THRESHOLD = 0.8;
constexpr double MIN_CLEARANCE_RADIUS = 0.000001;
constexpr int MAX_INTERPOLATION_DISTANCE = 2;
}  // namespace MedialAxis

// Canvas rendering and interaction parameters
namespace Rendering {
constexpr int DEFAULT_STROKE_WIDTH = 2;
constexpr int SELECTION_STROKE_WIDTH = 3;
constexpr int CONSTRUCTION_STROKE_WIDTH = 1;
constexpr int HIT_TEST_TOLERANCE = 5;
}  // namespace Rendering

// Display formatting and coordinate system parameters
namespace Units {
constexpr int DISPLAY_PRECISION = 3;
constexpr int COORDINATE_SCALE = 1;
}  // namespace Units

}  // namespace Constants
}  // namespace ChipCarving

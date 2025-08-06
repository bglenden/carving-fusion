/**
 * VCarvePath.h
 *
 * Data structures for V-carve toolpath generation from medial axis data.
 * Provides 3D point representation with depth calculations for CNC V-carve operations.
 */

#pragma once

#include <vector>

#include "Point2D.h"

namespace ChipCarving {
namespace Geometry {

/**
 * Represents a single point along a V-carve toolpath with 3D coordinates
 */
struct VCarvePoint {
  Point2D position;        ///< (x, y) position in world coordinates (mm)
  double depth;            ///< Z-depth below sketch plane (mm, positive = down)
  double clearanceRadius;  ///< Original clearance radius from medial axis (mm)

  VCarvePoint(const Point2D& pos, double d, double clearance) : position(pos), depth(d), clearanceRadius(clearance) {}

  VCarvePoint() : position(0, 0), depth(0.0), clearanceRadius(0.0) {}
};

/**
 * Represents a continuous V-carve toolpath consisting of connected points
 */
struct VCarvePath {
  std::vector<VCarvePoint> points;  ///< Sequential points along this path
  double totalLength;               ///< Total 2D length of path in mm
  bool isClosed;                    ///< Whether path forms a closed loop

  VCarvePath() : totalLength(0.0), isClosed(false) {}

  /**
   * Calculate total 2D path length
   * @return Total length in mm
   */
  double calculateLength() const;

  /**
   * Get the deepest point along this path
   * @return Maximum depth value (positive = deepest cut)
   */
  double getMaxDepth() const;

  /**
   * Get the shallowest point along this path
   * @return Minimum depth value
   */
  double getMinDepth() const;

  /**
   * Check if path is valid (has at least 2 points)
   * @return true if path can be used for toolpath generation
   */
  bool isValid() const;
};

/**
 * Collection of V-carve paths with statistics
 */
struct VCarveResults {
  std::vector<VCarvePath> paths;  ///< Individual toolpaths

  // Statistics
  int totalPaths = 0;        ///< Number of generated paths
  int totalPoints = 0;       ///< Total points across all paths
  double totalLength = 0.0;  ///< Total length across all paths (mm)
  double maxDepth = 0.0;     ///< Deepest cut across all paths (mm)
  double minDepth = 0.0;     ///< Shallowest cut across all paths (mm)

  // Success/error status
  bool success = false;      ///< Whether generation succeeded
  std::string errorMessage;  ///< Error details if failed

  /**
   * Update statistics based on current paths
   */
  void updateStatistics();

  /**
   * Get formatted summary string for logging/UI
   * @return Human-readable results summary
   */
  std::string getSummary() const;
};

}  // namespace Geometry
}  // namespace ChipCarving

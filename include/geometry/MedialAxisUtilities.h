/**
 * MedialAxisUtilities.h
 *
 * Utility functions for processing medial axis data for CNC toolpath generation.
 * Provides sampling and interpolation of medial axis paths at regular intervals.
 */

#pragma once

#include <vector>

#include "Point2D.h"

namespace ChipCarving {
namespace Geometry {

/**
 * Represents a single sampled point along a medial axis path
 */
struct SampledMedialPoint {
    Point2D position;        ///< (x, y) position in world coordinates
    double clearanceRadius;  ///< Clearance radius (max tool radius) at this point

    SampledMedialPoint(const Point2D& pos, double clearance)
        : position(pos), clearanceRadius(clearance) {}
};

/**
 * Represents a single continuous path of sampled medial axis points
 */
struct SampledMedialPath {
    std::vector<SampledMedialPoint> points;  ///< Sampled points along this path
    double totalLength;                      ///< Total length of this path in mm

    SampledMedialPath() : totalLength(0.0) {}
};

/**
 * Sample multiple medial axis paths at regular intervals for toolpath generation.
 *
 * This function takes raw medial axis data (multiple chains with clearance radii)
 * and produces evenly-spaced sample points suitable for CNC V-carve toolpath generation.
 *
 * Features:
 * - Preserves multi-path topology (e.g., branching in triangles)
 * - Interpolates points along segments longer than 1.5mm
 * - Samples at regular intervals along each path
 * - Always includes path endpoints (even with zero clearance)
 *
 * @param chains Vector of chains, each chain is a vector of Point2D positions
 * @param clearanceRadii Vector of clearance radius vectors (parallel to chains)
 * @param targetSpacing Target spacing between sampled points in mm (default 1.0mm)
 * @return Vector of sampled paths, each containing evenly-spaced points with clearances
 */
std::vector<SampledMedialPath> sampleMedialAxisPaths(
    const std::vector<std::vector<Point2D>>& chains,
    const std::vector<std::vector<double>>& clearanceRadii, double targetSpacing = 1.0);

}  // namespace Geometry
}  // namespace ChipCarving
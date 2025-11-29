/**
 * VCarveCalculatorOptimization.cpp
 *
 * Path conversion and optimization functionality for V-carve calculations.
 * Split from VCarveCalculator.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <set>

#include "geometry/VCarveCalculator.h"

namespace ChipCarving {
namespace Geometry {

VCarvePath VCarveCalculator::convertSampledPath(const SampledMedialPath& sampledPath,
                                                const Adapters::MedialAxisParameters& params) {
  VCarvePath vcarvePath;

  if (sampledPath.points.empty()) {
    return vcarvePath;
  }

  // Convert each sampled point to V-carve point with depth calculation
  for (const auto& sampledPoint : sampledPath.points) {
    // Calculate V-carve depth for all clearance radii (including 0.0 for sharp
    // corners)
    double depth = calculateVCarveDepth(sampledPoint.clearanceRadius, params.toolAngle, params.maxVCarveDepth);

    // Always add points - even zero clearance points are important for corners
    VCarvePoint vcarvePoint(sampledPoint.position, depth, sampledPoint.clearanceRadius);
    vcarvePath.points.push_back(vcarvePoint);
  }

  // Update path properties
  vcarvePath.totalLength = vcarvePath.calculateLength();
  vcarvePath.isClosed = false;  // For now, treat all paths as open

  return vcarvePath;
}

std::vector<VCarvePath> VCarveCalculator::optimizePaths(const std::vector<VCarvePath>& paths,
                                                        const Adapters::MedialAxisParameters& /* params */) {
  if (paths.empty()) {
    return paths;
  }

  // For now, implement basic optimization: merge paths that can be connected
  std::vector<VCarvePath> optimizedPaths = paths;

  // Sort paths by length (longest first) to prioritize keeping long paths
  std::sort(optimizedPaths.begin(), optimizedPaths.end(),
            [](const VCarvePath& a, const VCarvePath& b) { return a.totalLength > b.totalLength; });

  // Attempt to merge connectable paths
  bool mergedAny = true;
  while (mergedAny && optimizedPaths.size() > 1) {
    mergedAny = false;

    for (size_t i = 0; i < optimizedPaths.size() && !mergedAny; ++i) {
      for (size_t j = i + 1; j < optimizedPaths.size() && !mergedAny; ++j) {
        if (canConnectPaths(optimizedPaths[i], optimizedPaths[j])) {
          // Merge paths j into i
          VCarvePath merged = mergePaths(optimizedPaths[i], optimizedPaths[j]);

          // Replace path i with merged, remove path j
          optimizedPaths[i] = merged;
          optimizedPaths.erase(optimizedPaths.begin() + j);
          mergedAny = true;
        }
      }
    }
  }

  return optimizedPaths;
}

bool VCarveCalculator::canConnectPaths(const VCarvePath& path1, const VCarvePath& path2, double tolerance) {
  if (!path1.isValid() || !path2.isValid()) {
    return false;
  }

  // Get endpoints of both paths
  const Point2D& p1_start = path1.points.front().position;
  const Point2D& p1_end = path1.points.back().position;
  const Point2D& p2_start = path2.points.front().position;
  const Point2D& p2_end = path2.points.back().position;

  // Check all possible connections
  double dist1 = std::sqrt(std::pow(p1_end.x - p2_start.x, 2) + std::pow(p1_end.y - p2_start.y, 2));
  double dist2 = std::sqrt(std::pow(p1_end.x - p2_end.x, 2) + std::pow(p1_end.y - p2_end.y, 2));
  double dist3 = std::sqrt(std::pow(p1_start.x - p2_start.x, 2) + std::pow(p1_start.y - p2_start.y, 2));
  double dist4 = std::sqrt(std::pow(p1_start.x - p2_end.x, 2) + std::pow(p1_start.y - p2_end.y, 2));

  return (dist1 <= tolerance || dist2 <= tolerance || dist3 <= tolerance || dist4 <= tolerance);
}

VCarvePath VCarveCalculator::mergePaths(const VCarvePath& path1, const VCarvePath& path2) {
  VCarvePath merged;

  if (!path1.isValid() || !path2.isValid()) {
    return merged;
  }

  // Get endpoints for connection analysis
  const Point2D& p1_start = path1.points.front().position;
  const Point2D& p1_end = path1.points.back().position;
  const Point2D& p2_start = path2.points.front().position;
  const Point2D& p2_end = path2.points.back().position;

  // Find best connection strategy
  double tolerance = 0.1;  // Connection tolerance

  // Case 1: path1.end -> path2.start
  double dist1 = std::sqrt(std::pow(p1_end.x - p2_start.x, 2) + std::pow(p1_end.y - p2_start.y, 2));
  if (dist1 <= tolerance) {
    merged.points = path1.points;
    merged.points.insert(merged.points.end(), path2.points.begin(), path2.points.end());
    merged.totalLength = merged.calculateLength();
    return merged;
  }

  // Case 2: path1.end -> path2.end (reverse path2)
  double dist2 = std::sqrt(std::pow(p1_end.x - p2_end.x, 2) + std::pow(p1_end.y - p2_end.y, 2));
  if (dist2 <= tolerance) {
    merged.points = path1.points;
    // Add path2 in reverse order
    for (auto it = path2.points.rbegin(); it != path2.points.rend(); ++it) {
      merged.points.push_back(*it);
    }
    merged.totalLength = merged.calculateLength();
    return merged;
  }

  // Case 3: path2.end -> path1.start (reverse path1)
  double dist3 = std::sqrt(std::pow(p1_start.x - p2_end.x, 2) + std::pow(p1_start.y - p2_end.y, 2));
  if (dist3 <= tolerance) {
    merged.points = path2.points;
    merged.points.insert(merged.points.end(), path1.points.begin(), path1.points.end());
    merged.totalLength = merged.calculateLength();
    return merged;
  }

  // Case 4: path2.start -> path1.start (reverse both)
  double dist4 = std::sqrt(std::pow(p1_start.x - p2_start.x, 2) + std::pow(p1_start.y - p2_start.y, 2));
  if (dist4 <= tolerance) {
    // Add path2 in reverse order
    for (auto it = path2.points.rbegin(); it != path2.points.rend(); ++it) {
      merged.points.push_back(*it);
    }
    merged.points.insert(merged.points.end(), path1.points.begin(), path1.points.end());
    merged.totalLength = merged.calculateLength();
    return merged;
  }

  // No valid connection found, return empty path
  return merged;
}

}  // namespace Geometry
}  // namespace ChipCarving

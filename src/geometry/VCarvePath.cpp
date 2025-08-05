/**
 * VCarvePath.cpp
 *
 * Implementation of V-carve toolpath data structures.
 */

#include "../../include/geometry/VCarvePath.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ChipCarving {
namespace Geometry {

double VCarvePath::calculateLength() const {
  if (points.size() < 2) {
    return 0.0;
  }

  double length = 0.0;
  for (size_t i = 0; i < points.size() - 1; ++i) {
    const auto& p1 = points[i].position;
    const auto& p2 = points[i + 1].position;

    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    length += std::sqrt(dx * dx + dy * dy);
  }

  return length;
}

double VCarvePath::getMaxDepth() const {
  if (points.empty()) {
    return 0.0;
  }

  double maxDepth = points[0].depth;
  for (const auto& point : points) {
    maxDepth = std::max(maxDepth, point.depth);
  }

  return maxDepth;
}

double VCarvePath::getMinDepth() const {
  if (points.empty()) {
    return 0.0;
  }

  double minDepth = points[0].depth;
  for (const auto& point : points) {
    minDepth = std::min(minDepth, point.depth);
  }

  return minDepth;
}

bool VCarvePath::isValid() const { return points.size() >= 2; }

void VCarveResults::updateStatistics() {
  totalPaths = static_cast<int>(paths.size());
  totalPoints = 0;
  totalLength = 0.0;
  maxDepth = 0.0;
  minDepth = 0.0;

  if (paths.empty()) {
    return;
  }

  // Initialize min/max with first path values
  bool firstPath = true;

  for (auto& path : paths) {
    // Update path's own length
    path.totalLength = path.calculateLength();

    totalPoints += static_cast<int>(path.points.size());
    totalLength += path.totalLength;

    if (!path.points.empty()) {
      double pathMax = path.getMaxDepth();
      double pathMin = path.getMinDepth();

      if (firstPath) {
        maxDepth = pathMax;
        minDepth = pathMin;
        firstPath = false;
      } else {
        maxDepth = std::max(maxDepth, pathMax);
        minDepth = std::min(minDepth, pathMin);
      }
    }
  }
}

std::string VCarveResults::getSummary() const {
  std::ostringstream oss;
  oss << "V-Carve Results: " << totalPaths << " paths, " << totalPoints
      << " points, " << static_cast<int>(totalLength) << "mm length, "
      << "depths " << static_cast<int>(minDepth * 10) / 10.0 << "-"
      << static_cast<int>(maxDepth * 10) / 10.0 << "mm";
  return oss.str();
}

}  // namespace Geometry
}  // namespace ChipCarving

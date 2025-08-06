/**
 * TriArcCore.cpp
 *
 * Core functionality for TriArc - constructor, basic getters, bounds, contains,
 * centroid Split from TriArc.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

#include "../../include/geometry/TriArc.h"
#include "../../src/adapters/IFusionInterface.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace ChipCarving::Geometry;

// Definition for static constexpr members (required for C++14)
constexpr double TriArc::MAX_BULGE;
constexpr double TriArc::MIN_BULGE;

TriArc::TriArc(const Point2D& v1, const Point2D& v2, const Point2D& v3, const std::array<double, 3>& bulges)
    : vertices_{v1, v2, v3}, bulgeFactors_(bulges) {
  // Ensure all bulge factors are negative (concave arcs only)
  for (size_t i = 0; i < 3; ++i) {
    if (bulgeFactors_[i] > 0) {
      bulgeFactors_[i] = -bulgeFactors_[i];  // Convert positive to negative
    }
  }

  // Clamp to valid range
  clampBulgeFactors();
}

std::vector<Point2D> TriArc::getVertices() const {
  return std::vector<Point2D>(vertices_.begin(), vertices_.end());
}

bool TriArc::contains(const Point2D& point) const {
  // Simple implementation: check if point is inside triangle formed by vertices
  // This is an approximation - a full implementation would need to handle
  // curved edges

  // Use barycentric coordinates to test triangle containment
  double denom = (vertices_[1].y - vertices_[2].y) * (vertices_[0].x - vertices_[2].x) +
                 (vertices_[2].x - vertices_[1].x) * (vertices_[0].y - vertices_[2].y);

  if (std::abs(denom) < 1e-10) {
    return false;  // Degenerate triangle
  }

  double a = ((vertices_[1].y - vertices_[2].y) * (point.x - vertices_[2].x) +
              (vertices_[2].x - vertices_[1].x) * (point.y - vertices_[2].y)) /
             denom;
  double b = ((vertices_[2].y - vertices_[0].y) * (point.x - vertices_[2].x) +
              (vertices_[0].x - vertices_[2].x) * (point.y - vertices_[2].y)) /
             denom;
  double c = 1 - a - b;

  return a >= 0 && b >= 0 && c >= 0;
}

Point2D TriArc::getCentroid() const {
  return Point2D((vertices_[0].x + vertices_[1].x + vertices_[2].x) / 3.0,
                 (vertices_[0].y + vertices_[1].y + vertices_[2].y) / 3.0);
}

Point2D TriArc::getVertex(int index) const {
  if (index < 0 || index >= 3) {
    throw std::out_of_range("Vertex index must be 0, 1, or 2");
  }
  return vertices_[index];
}

double TriArc::getBulgeFactor(int arcIndex) const {
  if (arcIndex < 0 || arcIndex >= 3) {
    throw std::out_of_range("Arc index must be 0, 1, or 2");
  }
  return bulgeFactors_[arcIndex];
}

Point2D TriArc::getCenter() const {
  return getCentroid();  // For now, use centroid as center
}

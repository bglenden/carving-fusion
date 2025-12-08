/**
 * Basic 2D point structure and utilities for chip carving geometry
 */

#pragma once

#include <cmath>  // IWYU pragma: keep

namespace ChipCarving {
namespace Geometry {

/// Tolerance for floating-point geometric comparisons
static constexpr double GEOMETRY_EPSILON = 1e-9;

/**
 * Simple 2D point structure
 */
struct Point2D {
  double x;
  double y;

  Point2D(double x = 0.0, double y = 0.0) : x(x), y(y) {}

  // Explicitly defaulted special member functions with noexcept for move operations
  Point2D(const Point2D&) = default;
  Point2D& operator=(const Point2D&) = default;
  Point2D(Point2D&&) noexcept = default;
  Point2D& operator=(Point2D&&) noexcept = default;
  ~Point2D() = default;

  // Basic operators
  Point2D operator+(const Point2D& other) const {
    return Point2D(x + other.x, y + other.y);
  }

  Point2D operator-(const Point2D& other) const {
    return Point2D(x - other.x, y - other.y);
  }

  Point2D operator*(double scalar) const {
    return Point2D(x * scalar, y * scalar);
  }

  // Equality with tolerance
  bool equals(const Point2D& other, double tolerance = GEOMETRY_EPSILON) const {
    return std::abs(x - other.x) < tolerance && std::abs(y - other.y) < tolerance;
  }
};

// Utility functions
/**
 * Calculate distance between two points
 */
inline double distance(const Point2D& p1, const Point2D& p2) {
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  return std::sqrt(dx * dx + dy * dy);
}

/**
 * Calculate midpoint between two points
 */
inline Point2D midpoint(const Point2D& p1, const Point2D& p2) {
  return Point2D((p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0);
}

/**
 * Calculate perpendicular unit vector (90° CCW rotation from p1->p2)
 * Returns zero vector if points are too close
 */
inline Point2D perpendicular(const Point2D& p1, const Point2D& p2) {
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  double len = std::sqrt(dx * dx + dy * dy);

  if (len < GEOMETRY_EPSILON) {
    return Point2D(0.0, 0.0);
  }

  // 90° CCW rotation: (x, y) -> (-y, x), then normalize
  return Point2D(-dy / len, dx / len);
}

/**
 * Rotate a point around a center by the given angle (radians)
 */
inline Point2D rotatePoint(const Point2D& point, double angle, const Point2D& center) {
  double cos_a = std::cos(angle);
  double sin_a = std::sin(angle);

  // Translate to origin
  double x = point.x - center.x;
  double y = point.y - center.y;

  // Rotate
  double new_x = x * cos_a - y * sin_a;
  double new_y = x * sin_a + y * cos_a;

  // Translate back
  return Point2D(new_x + center.x, new_y + center.y);
}

}  // namespace Geometry
}  // namespace ChipCarving

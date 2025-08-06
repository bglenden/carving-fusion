/**
 * Point3D.h
 *
 * 3D point representation for V-carve toolpath generation.
 * Extends the 2D point system with Z-depth coordinates.
 */

#pragma once

#include "Point2D.h"

namespace ChipCarving {
namespace Geometry {

/**
 * Represents a point in 3D space with utility methods
 */
struct Point3D {
  double x, y, z;

  // Constructors
  Point3D() : x(0), y(0), z(0) {}
  Point3D(double x_val, double y_val, double z_val) : x(x_val), y(y_val), z(z_val) {}
  Point3D(const Point2D& point2d, double z_val) : x(point2d.x), y(point2d.y), z(z_val) {}

  // Equality operators
  bool operator==(const Point3D& other) const {
    const double epsilon = 1e-10;
    return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon && std::abs(z - other.z) < epsilon;
  }

  bool operator!=(const Point3D& other) const {
    return !(*this == other);
  }

  // Arithmetic operators
  Point3D operator+(const Point3D& other) const {
    return Point3D(x + other.x, y + other.y, z + other.z);
  }

  Point3D operator-(const Point3D& other) const {
    return Point3D(x - other.x, y - other.y, z - other.z);
  }

  Point3D operator*(double scalar) const {
    return Point3D(x * scalar, y * scalar, z * scalar);
  }

  // Distance calculations
  double distance(const Point3D& other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    double dz = z - other.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }

  double distance2D(const Point3D& other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
  }

  // Get 2D projection
  Point2D to2D() const {
    return Point2D(x, y);
  }

  // Length/magnitude
  double magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
  }

  // Normalize to unit vector
  Point3D normalize() const {
    double mag = magnitude();
    if (mag < 1e-10) {
      return Point3D(0, 0, 0);
    }
    return Point3D(x / mag, y / mag, z / mag);
  }
};

}  // namespace Geometry
}  // namespace ChipCarving

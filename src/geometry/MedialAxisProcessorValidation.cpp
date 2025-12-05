/**
 * MedialAxisProcessorValidation.cpp
 *
 * Polygon validation for MedialAxisProcessor
 * Split from MedialAxisProcessorCore.cpp for maintainability
 */

#include <cmath>
#include <string>

#include "geometry/MedialAxisProcessor.h"

namespace ChipCarving {
namespace Geometry {
namespace {

// Helper function to check if two line segments intersect
// Based on the orientation method
bool doSegmentsIntersect(const Point2D& p1, const Point2D& q1, const Point2D& p2, const Point2D& q2) {
  auto orientation = [](const Point2D& p, const Point2D& q, const Point2D& r) -> int {
    double val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (std::abs(val) < 1e-10)
      return 0;                // Collinear
    return (val > 0) ? 1 : 2;  // Clockwise or Counterclockwise
  };

  auto onSegment = [](const Point2D& p, const Point2D& q, const Point2D& r) -> bool {
    return q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) && q.y <= std::max(p.y, r.y) &&
           q.y >= std::min(p.y, r.y);
  };

  int o1 = orientation(p1, q1, p2);
  int o2 = orientation(p1, q1, q2);
  int o3 = orientation(p2, q2, p1);
  int o4 = orientation(p2, q2, q1);

  // General case
  if (o1 != o2 && o3 != o4)
    return true;

  // Special Cases
  // p1, q1 and p2 are collinear and p2 lies on segment p1q1
  if (o1 == 0 && onSegment(p1, p2, q1))
    return true;

  // p1, q1 and q2 are collinear and q2 lies on segment p1q1
  if (o2 == 0 && onSegment(p1, q2, q1))
    return true;

  // p2, q2 and p1 are collinear and p1 lies on segment p2q2
  if (o3 == 0 && onSegment(p2, p1, q2))
    return true;

  // p2, q2 and q1 are collinear and q1 lies on segment p2q2
  if (o4 == 0 && onSegment(p2, q1, q2))
    return true;

  return false;  // Doesn't fall in any of the above cases
}

}  // namespace

bool MedialAxisProcessor::validatePolygonForOpenVoronoi(const std::vector<Point2D>& polygon) {
  log("=== POLYGON VALIDATION START ===");
  log("Validating polygon with " + std::to_string(polygon.size()) + " vertices for OpenVoronoi");

  if (polygon.size() < 3) {
    log("ERROR: Polygon must have at least 3 vertices, got " + std::to_string(polygon.size()));
    return false;
  }

  // Check if polygon is closed (last vertex should NOT equal first for our algorithm)
  // Fusion gives us properly closed polygons without duplicate vertices
  bool lastEqualsFirst = false;
  if (polygon.size() > 3) {
    double dist = std::sqrt(std::pow(polygon.back().x - polygon.front().x, 2) +
                            std::pow(polygon.back().y - polygon.front().y, 2));
    lastEqualsFirst = (dist < 1e-10);
  }

  if (lastEqualsFirst) {
    log("Warning: Last vertex equals first vertex - polygon appears to have duplicate closing vertex");
    log("This may indicate improper polygon construction");
  }

  // Check for self-intersections
  // We need to test each edge against every non-adjacent edge
  log("Checking for self-intersections...");
  size_t numEdges = polygon.size();
  int intersectionCount = 0;
  const int MAX_INTERSECTIONS_TO_LOG = 5;

  for (size_t i = 0; i < numEdges; ++i) {
    Point2D p1 = polygon[i];
    Point2D q1 = polygon[(i + 1) % numEdges];

    // Test against all non-adjacent edges
    for (size_t j = i + 2; j < numEdges; ++j) {
      // Special case: don't test edge i against edge (numEdges-1) when i = 0
      // since they are adjacent in a closed polygon
      if (i == 0 && j == numEdges - 1) {
        continue;
      }

      Point2D p2 = polygon[j];
      Point2D q2 = polygon[(j + 1) % numEdges];

      if (doSegmentsIntersect(p1, q1, p2, q2)) {
        intersectionCount++;
        if (intersectionCount <= MAX_INTERSECTIONS_TO_LOG) {
          log("Self-intersection detected: Edge " + std::to_string(i) + "-" + std::to_string((i + 1) % numEdges) +
              " intersects edge " + std::to_string(j) + "-" + std::to_string((j + 1) % numEdges));
          log("  Edge 1: (" + std::to_string(p1.x) + ", " + std::to_string(p1.y) + ") to (" + std::to_string(q1.x) +
              ", " + std::to_string(q1.y) + ")");
          log("  Edge 2: (" + std::to_string(p2.x) + ", " + std::to_string(p2.y) + ") to (" + std::to_string(q2.x) +
              ", " + std::to_string(q2.y) + ")");
        } else if (intersectionCount == MAX_INTERSECTIONS_TO_LOG + 1) {
          log("... (additional self-intersections not logged)");
        }
      }
    }
  }

  if (intersectionCount > 0) {
    log("ERROR: Polygon has " + std::to_string(intersectionCount) +
        " self-intersections - OpenVoronoi requires simple polygons");
    return false;
  }

  log("Self-intersection check passed - no self-intersections detected");

  // Check for degenerate edges (zero length)
  log("Checking for degenerate edges...");
  int degenerateCount = 0;
  for (size_t i = 0; i < numEdges; ++i) {
    Point2D p1 = polygon[i];
    Point2D p2 = polygon[(i + 1) % numEdges];
    double edgeLength = std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));

    if (edgeLength < 1e-10) {
      log("ERROR: Degenerate edge " + std::to_string(i) + " between (" + std::to_string(p1.x) + ", " +
          std::to_string(p1.y) + ") and (" + std::to_string(p2.x) + ", " + std::to_string(p2.y) +
          ") length: " + std::to_string(edgeLength));
      degenerateCount++;
      if (degenerateCount >= 3) {
        log("... (additional degenerate edges not logged)");
        return false;
      }
    }
  }

  if (degenerateCount > 0) {
    log("ERROR: " + std::to_string(degenerateCount) + " degenerate edges detected");
    return false;
  }

  log("Degenerate edge check passed - all edges have sufficient length");

  // Check if all points are within unit circle
  log("Checking if all points are within unit circle...");
  int outsideCount = 0;
  for (size_t i = 0; i < polygon.size(); ++i) {
    double distance = std::sqrt(polygon[i].x * polygon[i].x + polygon[i].y * polygon[i].y);
    if (distance > 1.0) {
      log("ERROR: Point " + std::to_string(i) + " at (" + std::to_string(polygon[i].x) + ", " +
          std::to_string(polygon[i].y) + ") is outside unit circle (distance: " + std::to_string(distance) + ")");
      outsideCount++;
      if (outsideCount >= 3) {
        log("... (additional points outside unit circle not logged)");
        return false;
      }
    }
  }

  if (outsideCount > 0) {
    log("ERROR: " + std::to_string(outsideCount) + " points are outside unit circle");
    return false;
  }

  log("Unit circle check passed - all points within circle");

  // Check for zero-area polygon (collinear points)
  // Use the shoelace formula to compute signed area
  log("Checking for degenerate (zero-area) polygon...");
  double signedArea = 0.0;
  for (size_t i = 0; i < polygon.size(); ++i) {
    size_t j = (i + 1) % polygon.size();
    signedArea += polygon[i].x * polygon[j].y;
    signedArea -= polygon[j].x * polygon[i].y;
  }
  signedArea = std::abs(signedArea) / 2.0;

  // For a polygon scaled to unit circle, a reasonable minimum area threshold
  // is based on the minimum edge length squared
  double minArea = 1e-10;
  if (signedArea < minArea) {
    log("ERROR: Polygon has near-zero area (" + std::to_string(signedArea) +
        ") - points may be collinear or nearly collinear");
    return false;
  }
  log("Area check passed - polygon has sufficient area: " + std::to_string(signedArea));

  log("=== POLYGON VALIDATION PASSED ===");
  return true;
}

}  // namespace Geometry
}  // namespace ChipCarving

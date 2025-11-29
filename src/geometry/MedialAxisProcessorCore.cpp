/**
 * MedialAxisProcessorCore.cpp
 *
 * Core functionality for MedialAxisProcessor
 * Split from MedialAxisProcessor.cpp for maintainability
 *
 * Note: validatePolygonForOpenVoronoi() is in MedialAxisProcessorValidation.cpp
 */

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <streambuf>

#include "geometry/MedialAxisProcessor.h"
#include "geometry/Shape.h"
#include "utils/logging.h"

namespace ChipCarving {
namespace Geometry {

MedialAxisProcessor::MedialAxisProcessor()
    : polygonTolerance_(0.25), medialThreshold_(0.8), verbose_(false), medialAxisWalkPoints_(0) {}

MedialAxisProcessor::MedialAxisProcessor(double polygonTolerance, double medialThreshold)
    : polygonTolerance_(polygonTolerance),
      medialThreshold_(medialThreshold),
      verbose_(false),
      medialAxisWalkPoints_(0) {}

MedialAxisResults MedialAxisProcessor::computeMedialAxis(const Shape& shape) {
  (void)shape;  // Suppress unused parameter warning - this function is deprecated
  log("ERROR: Shape-based medial axis computation is deprecated!");
  log("Polygonization must be done via FusionAPIAdapter::extractProfileVertices()");
  log("This ensures geometry comes from actual Fusion profiles, not original shape parameters");

  MedialAxisResults results;
  results.success = false;
  results.errorMessage = "Shape-based polygonization is deprecated. Use Fusion profile extraction instead.";
  return results;
}

MedialAxisResults MedialAxisProcessor::computeMedialAxis(const std::vector<Point2D>& polygon) {
  // Debug output to verify function is called
  LOG_DEBUG("computeMedialAxis called with " << polygon.size() << " vertices");

  log("[MedialAxisProcessor] computeMedialAxis called with " + std::to_string(polygon.size()) + " vertices");
  MedialAxisResults results;

  if (polygon.size() < 3) {
    results.errorMessage = "Polygon must have at least 3 vertices";
    log("Error: " + results.errorMessage);
    return results;
  }

  // Validate polygon doesn't have duplicate consecutive vertices
  for (size_t i = 0; i < polygon.size() - 1; ++i) {
    size_t next = i + 1;

    // Special case: Allow last vertex to match first vertex (closed polygon)
    if (i == polygon.size() - 2 && next == polygon.size() - 1) {
      // Check if last vertex matches first vertex
      double distToFirst =
          std::sqrt(std::pow(polygon[next].x - polygon[0].x, 2) + std::pow(polygon[next].y - polygon[0].y, 2));
      if (distToFirst < 1e-10) {
        // This is a properly closed polygon, skip this check
        continue;
      }
    }

    double dist = std::sqrt(std::pow(polygon[i].x - polygon[next].x, 2) + std::pow(polygon[i].y - polygon[next].y, 2));
    if (dist < 1e-10) {
      results.errorMessage = "Polygon has duplicate consecutive vertices at index " + std::to_string(i);
      log("Error: " + results.errorMessage);
      return results;
    }
  }

  log("Computing medial axis for polygon with " + std::to_string(polygon.size()) + " vertices");

  // Transform to unit circle
  std::vector<Point2D> transformedPolygon = transformToUnitCircle(polygon, results.transform);

  if (verbose_) {
    log("Original bounds: (" + std::to_string(results.transform.originalMin.x) + ", " +
        std::to_string(results.transform.originalMin.y) + ") to (" + std::to_string(results.transform.originalMax.x) +
        ", " + std::to_string(results.transform.originalMax.y) + ")");
    log("Scale factor: " + std::to_string(results.transform.scale));
    log("Offset: (" + std::to_string(results.transform.offset.x) + ", " + std::to_string(results.transform.offset.y) +
        ")");
  }

  // Validate for OpenVoronoi
  if (!validatePolygonForOpenVoronoi(transformedPolygon)) {
    results.errorMessage = "Polygon failed validation for OpenVoronoi computation";
    log("Error: " + results.errorMessage);
    return results;
  }

  // Compute medial axis using OpenVoronoi
  if (!computeOpenVoronoi(transformedPolygon, results)) {
    // Error already logged in computeOpenVoronoi
    return results;
  }

  results.success = true;
  log("Medial axis computation successful");

  return results;
}

std::vector<Point2D> MedialAxisProcessor::transformToUnitCircle(const std::vector<Point2D>& polygon,
                                                                TransformParams& transform) {
  if (polygon.empty()) {
    log("Warning: transformToUnitCircle called with empty polygon");
    return polygon;
  }

  // Find bounding box
  transform.originalMin = polygon[0];
  transform.originalMax = polygon[0];

  for (const auto& point : polygon) {
    transform.originalMin.x = std::min(transform.originalMin.x, point.x);
    transform.originalMin.y = std::min(transform.originalMin.y, point.y);
    transform.originalMax.x = std::max(transform.originalMax.x, point.x);
    transform.originalMax.y = std::max(transform.originalMax.y, point.y);
  }

  // Calculate center and max dimension
  double centerX = (transform.originalMin.x + transform.originalMax.x) / 2.0;
  double centerY = (transform.originalMin.y + transform.originalMax.y) / 2.0;
  double width = transform.originalMax.x - transform.originalMin.x;
  double height = transform.originalMax.y - transform.originalMin.y;
  double maxDimension = std::max(width, height);

  // Apply safety margin to ensure we stay within unit circle
  const double SAFETY_MARGIN = 0.85;  // Use 85% of unit circle
  transform.scale = (maxDimension > 0) ? SAFETY_MARGIN / maxDimension : 1.0;
  transform.offset.x = centerX;
  transform.offset.y = centerY;

  // Transform each point
  std::vector<Point2D> transformed;
  transformed.reserve(polygon.size());

  for (const auto& point : polygon) {
    Point2D transformedPoint;
    transformedPoint.x = (point.x - centerX) * transform.scale;
    transformedPoint.y = (point.y - centerY) * transform.scale;
    transformed.push_back(transformedPoint);
  }

  // Verify all points are within unit circle
  for (const auto& point : transformed) {
    double distance = std::sqrt(point.x * point.x + point.y * point.y);
    if (distance > 1.0) {
      log("Warning: Transformed point distance " + std::to_string(distance) + " exceeds unit circle");
    }
  }

  return transformed;
}

Point2D MedialAxisProcessor::transformFromUnitCircle(const Point2D& unitPoint, const TransformParams& transform) {
  // Reverse scaling then translation
  Point2D scaled = Point2D(unitPoint.x / transform.scale, unitPoint.y / transform.scale);
  Point2D worldPoint = Point2D(scaled.x + transform.offset.x, scaled.y + transform.offset.y);
  return worldPoint;
}

}  // namespace Geometry
}  // namespace ChipCarving

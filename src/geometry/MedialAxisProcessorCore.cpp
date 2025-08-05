/**
 * MedialAxisProcessorCore.cpp
 *
 * Core functionality for MedialAxisProcessor
 * Split from MedialAxisProcessor.cpp for maintainability
 */

#include "../../include/geometry/MedialAxisProcessor.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <streambuf>

#include "../../include/geometry/Shape.h"
#include "../../include/utils/logging.h"

namespace ChipCarving {
namespace Geometry {

// Helper function to check if two line segments intersect
// Based on the orientation method
static bool doSegmentsIntersect(const Point2D& p1, const Point2D& q1, const Point2D& p2,
                                const Point2D& q2) {
    auto orientation = [](const Point2D& p, const Point2D& q, const Point2D& r) -> int {
        double val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
        if (std::abs(val) < 1e-10)
            return 0;              // Collinear
        return (val > 0) ? 1 : 2;  // Clockwise or Counterclockwise
    };

    auto onSegment = [](const Point2D& p, const Point2D& q, const Point2D& r) -> bool {
        if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) && q.y <= std::max(p.y, r.y) &&
            q.y >= std::min(p.y, r.y))
            return true;
        return false;
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

MedialAxisProcessor::MedialAxisProcessor()
    : polygonTolerance_(0.25), medialThreshold_(0.8), verbose_(false), medialAxisWalkPoints_(0) {}

MedialAxisProcessor::MedialAxisProcessor(double polygonTolerance, double medialThreshold)
    : polygonTolerance_(polygonTolerance), medialThreshold_(medialThreshold), verbose_(false), medialAxisWalkPoints_(0) {}

MedialAxisResults MedialAxisProcessor::computeMedialAxis(const Shape& shape) {
    (void)shape;  // Suppress unused parameter warning - this function is deprecated
    log("ERROR: Shape-based medial axis computation is deprecated!");
    log("Polygonization must be done via FusionAPIAdapter::extractProfileVertices()");
    log("This ensures geometry comes from actual Fusion profiles, not original shape parameters");

    MedialAxisResults results;
    results.success = false;
    results.errorMessage =
        "Shape-based polygonization is deprecated. Use Fusion profile extraction instead.";
    return results;
}

MedialAxisResults MedialAxisProcessor::computeMedialAxis(const std::vector<Point2D>& polygon) {
    // Debug output to verify function is called
    LOG_DEBUG("computeMedialAxis called with " << polygon.size() << " vertices");

    log("[MedialAxisProcessor] computeMedialAxis called with " + std::to_string(polygon.size()) +
        " vertices");
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
            double distToFirst = std::sqrt(std::pow(polygon[next].x - polygon[0].x, 2) +
                                           std::pow(polygon[next].y - polygon[0].y, 2));
            if (distToFirst < 1e-10) {
                // This is a properly closed polygon, skip this check
                continue;
            }
        }

        double dist = std::sqrt(std::pow(polygon[i].x - polygon[next].x, 2) +
                                std::pow(polygon[i].y - polygon[next].y, 2));
        if (dist < 1e-10) {
            results.errorMessage =
                "Polygon has duplicate consecutive vertices at index " + std::to_string(i);
            log("Error: " + results.errorMessage);
            return results;
        }
    }

    log("Computing medial axis for polygon with " + std::to_string(polygon.size()) + " vertices");

    // Transform to unit circle
    std::vector<Point2D> transformedPolygon = transformToUnitCircle(polygon, results.transform);

    if (verbose_) {
        log("Original bounds: (" + std::to_string(results.transform.originalMin.x) + ", " +
            std::to_string(results.transform.originalMin.y) + ") to (" +
            std::to_string(results.transform.originalMax.x) + ", " +
            std::to_string(results.transform.originalMax.y) + ")");
        log("Scale factor: " + std::to_string(results.transform.scale));
        log("Offset: (" + std::to_string(results.transform.offset.x) + ", " +
            std::to_string(results.transform.offset.y) + ")");
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
            log("Warning: Transformed point distance " + std::to_string(distance) +
                " exceeds unit circle");
        }
    }

    return transformed;
}

Point2D MedialAxisProcessor::transformFromUnitCircle(const Point2D& unitPoint,
                                                     const TransformParams& transform) {
    // Reverse scaling then translation
    Point2D scaled = Point2D(unitPoint.x / transform.scale, unitPoint.y / transform.scale);
    Point2D worldPoint = Point2D(scaled.x + transform.offset.x, scaled.y + transform.offset.y);
    return worldPoint;
}

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
        log("Warning: Last vertex equals first vertex - polygon appears to have duplicate "
            "closing vertex");
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
                    log("Self-intersection detected: Edge " + std::to_string(i) + "-" +
                        std::to_string((i + 1) % numEdges) + " intersects edge " + std::to_string(j) +
                        "-" + std::to_string((j + 1) % numEdges));
                    log("  Edge 1: (" + std::to_string(p1.x) + ", " + std::to_string(p1.y) + ") to (" +
                        std::to_string(q1.x) + ", " + std::to_string(q1.y) + ")");
                    log("  Edge 2: (" + std::to_string(p2.x) + ", " + std::to_string(p2.y) + ") to (" +
                        std::to_string(q2.x) + ", " + std::to_string(q2.y) + ")");
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
            log("ERROR: Degenerate edge " + std::to_string(i) + " between (" + 
                std::to_string(p1.x) + ", " + std::to_string(p1.y) + ") and (" +
                std::to_string(p2.x) + ", " + std::to_string(p2.y) + ") length: " + 
                std::to_string(edgeLength));
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
        double distance =
            std::sqrt(polygon[i].x * polygon[i].x + polygon[i].y * polygon[i].y);
        if (distance > 1.0) {
            log("ERROR: Point " + std::to_string(i) + " at (" + std::to_string(polygon[i].x) + 
                ", " + std::to_string(polygon[i].y) + ") is outside unit circle (distance: " +
                std::to_string(distance) + ")");
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

    log("=== POLYGON VALIDATION PASSED ===");
    return true;
}

}  // namespace Geometry
}  // namespace ChipCarving

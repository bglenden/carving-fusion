/**
 * Base shape interface for chip carving geometry
 */

#pragma once

#include <vector>

#include "Point2D.h"

namespace ChipCarving {

// Forward declarations
namespace Adapters {
class ISketch;
class ILogger;
}  // namespace Adapters

namespace Geometry {

/**
 * Abstract base class for all chip carving shapes
 */
class Shape {
   public:
    virtual ~Shape() = default;

    /**
     * Get the vertices that define the shape
     */
    virtual std::vector<Point2D> getVertices() const = 0;

    /**
     * Get polygon approximation of the shape suitable for OpenVoronoi processing
     * NOTE: This method is deprecated. Polygonization is now handled by
     * FusionAPIAdapter::extractProfileVertices() using Fusion's strokes API
     * to ensure accuracy with user-edited geometry.
     * @param maxError Maximum allowed error between curved edge and polygon approximation (default
     * 0.25mm)
     * @return Vector of points representing polygon approximation in counterclockwise order
     */
    virtual std::vector<Point2D> getPolygonVertices(double maxError = 0.25) const {
        // Default implementation returns vertices - shapes should not implement polygonization
        (void)maxError;  // Suppress unused parameter warning
        return getVertices();
    }

    /**
     * Draw the shape to a Fusion 360 sketch using the provided adapter
     */
    virtual void drawToSketch(Adapters::ISketch* sketch,
                              Adapters::ILogger* logger = nullptr) const = 0;


    /**
     * Check if a point is inside the shape
     * @param point The point to test
     * @return true if the point is inside or on the boundary
     */
    virtual bool contains(const Point2D& point) const = 0;

    /**
     * Get the centroid (geometric center) of the shape
     */
    virtual Point2D getCentroid() const = 0;
};

/**
 * Utility functions for shape operations
 */


/**
 * Calculate centroid of a polygon defined by vertices
 */
inline Point2D calculateCentroid(const std::vector<Point2D>& vertices) {
    if (vertices.empty()) {
        return Point2D(0.0, 0.0);
    }

    double sum_x = 0.0;
    double sum_y = 0.0;
    for (const auto& vertex : vertices) {
        sum_x += vertex.x;
        sum_y += vertex.y;
    }

    return Point2D(sum_x / vertices.size(), sum_y / vertices.size());
}

/**
 * Calculate maximum distance from chord to arc for polygonization error estimation
 * @param start Start point of chord
 * @param end End point of chord
 * @param center Arc center point
 * @param radius Arc radius
 * @return Maximum perpendicular distance from chord line to arc
 */
inline double calculateChordToArcError(const Point2D& start, const Point2D& end,
                                       const Point2D& center, double radius) {
    // Calculate chord midpoint
    Point2D chordMid = Point2D((start.x + end.x) * 0.5, (start.y + end.y) * 0.5);

    // Calculate point on arc at chord midpoint
    Point2D toChordMid = chordMid - center;
    double distToChordMid = distance(Point2D(0, 0), toChordMid);

    if (distToChordMid < 1e-9) {
        // Center coincides with chord midpoint - degenerate case
        return 0.0;
    }

    // Point on arc is at radius distance from center in direction of chord midpoint
    Point2D arcMid = center + Point2D(toChordMid.x / distToChordMid * radius,
                                      toChordMid.y / distToChordMid * radius);

    // Distance between chord midpoint and arc midpoint is the error
    return distance(chordMid, arcMid);
}

}  // namespace Geometry
}  // namespace ChipCarving
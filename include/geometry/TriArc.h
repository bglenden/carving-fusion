/**
 * TriArc shape implementation
 * Represents a triangle with curved edges defined by bulge factors
 * Based on TypeScript implementation from design_program/src/shapes/TriArc.ts
 */

#pragma once

#include <array>

#include "Point2D.h"
#include "Shape.h"

namespace ChipCarving {
namespace Geometry {

/**
 * Arc parameters for Fusion 360 drawing
 */
struct ArcParams {
  Point2D center;
  double radius;
  double startAngle;
  double endAngle;
  bool anticlockwise;  // True for CCW sweep direction

  ArcParams() : center(0, 0), radius(0), startAngle(0), endAngle(0), anticlockwise(false) {}
  ArcParams(Point2D c, double r, double start, double end, bool ccw)
      : center(c), radius(r), startAngle(start), endAngle(end), anticlockwise(ccw) {}
};

/**
 * TriArc shape with three vertices and three curved edges
 * Each edge is defined by a bulge factor controlling curvature
 *
 * Key constraints:
 * - Bulge factors must be negative (concave arcs only)
 * - Default bulge factors: [-0.125, -0.125, -0.125]
 * - Bulge factor range: [-0.99, -0.01]
 * - Bulge factor = (sagitta × 2) / chord_length
 */
class TriArc : public Shape {
 private:
  std::array<Point2D, 3> vertices_{};
  std::array<double, 3> bulgeFactors_{};

  static constexpr double DEFAULT_BULGE = -0.125;
  static constexpr double MIN_BULGE = -0.2;
  static constexpr double MAX_BULGE = -0.001;  // Allow smaller values for nearly straight edges
  static constexpr double EPSILON = 1e-9;

 public:
  /**
   * Create TriArc with three vertices and optional bulge factors
   * @param v1 First vertex
   * @param v2 Second vertex
   * @param v3 Third vertex
   * @param bulges Array of 3 bulge factors (must be negative for concave arcs)
   */
  TriArc(const Point2D& v1, const Point2D& v2, const Point2D& v3,
         const std::array<double, 3>& bulges = {DEFAULT_BULGE, DEFAULT_BULGE, DEFAULT_BULGE});

  // Shape interface implementation
  std::vector<Point2D> getVertices() const override;
  // NOTE: getPolygonVertices() removed - polygonization now handled by Fusion strokes
  void drawToSketch(Adapters::ISketch* sketch, Adapters::ILogger* logger) const override;
  bool contains(const Point2D& point) const override;
  Point2D getCentroid() const override;

  // TriArc-specific accessors
  Point2D getVertex(int index) const;
  double getBulgeFactor(int arcIndex) const;
  const std::array<double, 3>& getBulgeFactors() const {
    return bulgeFactors_;
  }

  /**
   * Get triangle centroid (center point)
   */
  Point2D getCenter() const;

  /**
   * Get arc parameters for each edge (for Fusion 360 drawing)
   * @return Array of 3 ArcParams, one for each edge
   */
  std::array<ArcParams, 3> getArcParameters() const;

  /**
   * Get arc parameters for specific edge
   * @param arcIndex Edge index (0=v1→v2, 1=v2→v3, 2=v3→v1)
   */
  ArcParams getArcParameters(int arcIndex) const;

  /**
   * Calculate sagitta (arc height) from bulge factor and chord length
   * @param bulge Bulge factor (negative for concave)
   * @param chordLength Length of chord
   * @return Sagitta distance (always positive)
   */
  static double sagittaFromBulge(double bulge, double chordLength);

  /**
   * Calculate bulge factor from sagitta and chord length
   * @param sagitta Arc height (positive)
   * @param chordLength Length of chord
   * @return Bulge factor (negative for concave)
   */
  static double bulgeFromSagitta(double sagitta, double chordLength);

  /**
   * Get chord midpoint for specific edge
   * @param arcIndex Edge index (0=v1→v2, 1=v2→v3, 2=v3→v1)
   */
  Point2D getChordMidpoint(int arcIndex) const;

  /**
   * Get perpendicular normal vector pointing toward triangle centroid
   * @param arcIndex Edge index (0=v1→v2, 1=v2→v3, 2=v3→v1)
   */
  Point2D getPerpendicularNormal(int arcIndex) const;

  /**
   * Validate that all bulge factors are in valid range and negative
   * @return True if all bulge factors are valid
   */
  bool hasValidBulgeFactors() const;

  /**
   * Clamp bulge factors to valid range [-0.99, -0.01]
   */
  void clampBulgeFactors();

  /**
   * Check if edge should be drawn as straight line (tiny bulge factor)
   * @param arcIndex Edge index
   */
  bool isEdgeStraight(int arcIndex) const;

  /**
   * Get chord length for specific edge
   * @param arcIndex Edge index (0=v1→v2, 1=v2→v3, 2=v3→v1)
   */
  double getChordLength(int arcIndex) const;

 private:
  /**
   * Calculate arc center for specific edge using bulge factor
   * @param p1 Start point of edge
   * @param p2 End point of edge
   * @param bulgeFactor Bulge factor for this edge
   * @param arcIndex Arc index for normal vector calculation
   * @return Arc center point
   */
  Point2D calculateArcCenter(const Point2D& p1, const Point2D& p2, double bulgeFactor, int arcIndex) const;

  /**
   * Calculate arc radius from chord length and sagitta
   * @param chordLength Length of chord
   * @param sagitta Arc height
   * @return Arc radius
   */
  static double calculateRadius(double chordLength, double sagitta);

  /**
   * Calculate arc angles for drawing
   * @param center Arc center
   * @param p1 Start point
   * @param p2 End point
   * @param isConcave True if arc is concave (bulge < 0)
   * @return Pair of (startAngle, endAngle)
   */
  static std::pair<double, double> calculateArcAngles(const Point2D& center, const Point2D& p1, const Point2D& p2,
                                                      bool isConcave);
};

}  // namespace Geometry
}  // namespace ChipCarving

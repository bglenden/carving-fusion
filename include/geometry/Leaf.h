/**
 * Leaf shape - vesica piscis formed by two intersecting circles
 * Matches the TypeScript implementation in design_program
 */

#pragma once

#include <cmath>

#include "Shape.h"

namespace ChipCarving {
namespace Geometry {

/**
 * Leaf shape implementation
 * A vesica piscis formed by the intersection of two circles with the same radius,
 * each centered at one of the two focus points.
 */
class Leaf : public Shape {
 private:
  Point2D focus1_;
  Point2D focus2_;
  double radius_;

 public:
  /**
   * Constructor with automatic radius calculation
   * @param f1 First focus point
   * @param f2 Second focus point
   * @param radius Circle radius (if < 0, uses default calculation: 0.65 * chord_length)
   */
  Leaf(const Point2D& f1, const Point2D& f2, double radius = -1.0) : focus1_(f1), focus2_(f2) {
    if (radius < 0) {
      double dist = distance(f1, f2);
      radius_ = dist * 0.65;  // Default from TypeScript ShapeFactory
    } else {
      radius_ = radius;
    }
  }

  // Getters
  Point2D getFocus1() const {
    return focus1_;
  }
  Point2D getFocus2() const {
    return focus2_;
  }
  double getRadius() const {
    return radius_;
  }

  /**
   * Parameters needed to draw an arc in Fusion 360
   */
  struct ArcParams {
    Point2D center;
    double radius;
    double startAngle;
    double endAngle;
    bool anticlockwise;

    ArcParams(const Point2D& c, double r, double start, double end, bool ccw)
        : center(c), radius(r), startAngle(start), endAngle(end), anticlockwise(ccw) {}
  };

  /**
   * Calculate the centers of the two arcs that form the leaf
   * @return pair of arc centers (first arc center, second arc center)
   */
  std::pair<Point2D, Point2D> getArcCenters() const;

  /**
   * Get complete arc parameters for drawing both arcs in Fusion 360
   * @return pair of arc parameters for the two arcs that form the leaf
   */
  std::pair<ArcParams, ArcParams> getArcParameters() const;

  /**
   * Calculate the sagitta (distance from chord midpoint to arc peak)
   * Used for shape editing and verification
   */
  double getSagitta() const;

  /**
   * Check if the leaf geometry is valid (radius large enough for chord length)
   */
  bool isValidGeometry() const;

  // Shape interface implementation
  std::vector<Point2D> getVertices() const override {
    return {focus1_, focus2_};
  }

  // NOTE: getPolygonVertices() removed - polygonization now handled by Fusion strokes

  void drawToSketch(Adapters::ISketch* sketch, Adapters::ILogger* logger = nullptr) const override;
  bool contains(const Point2D& point) const override;
  Point2D getCentroid() const override;

 private:
  /**
   * Calculate distance from chord center to arc center (d_center in TypeScript)
   * Used internally for arc calculations
   */
  double getChordCenterToArcCenterDistance() const;
};

}  // namespace Geometry
}  // namespace ChipCarving

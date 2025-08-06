/**
 * Factory for creating Shape objects from JSON data
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Point2D.h"
#include "Shape.h"

namespace ChipCarving {

// Forward declarations
namespace Adapters {
class ILogger;
}

namespace Geometry {

/**
 * Factory class for creating Shape objects from parsed JSON data
 */
class ShapeFactory {
 public:
  /**
   * Create a shape from JSON shape object
   * @param shapeJson JSON string representing a single shape
   * @return Unique pointer to created shape
   * @throws std::runtime_error if shape type is unknown or data is invalid
   */
  static std::unique_ptr<Shape> createFromJson(const std::string& shapeJson, const Adapters::ILogger* logger = nullptr);

  /**
   * Create a Leaf shape from JSON data
   * @param vertices Array of 2 points (foci)
   * @param radius Arc radius
   * @return Unique pointer to Leaf shape
   */
  static std::unique_ptr<Shape> createLeaf(const std::vector<Point2D>& vertices, double radius,
                                           const Adapters::ILogger* logger = nullptr);

  /**
   * Create a TriArc shape from JSON data
   * @param vertices Array of 3 points (triangle vertices)
   * @param curvatures Array of 3 curvature values (bulge factors)
   * @return Unique pointer to TriArc shape
   */
  static std::unique_ptr<Shape> createTriArc(const std::vector<Point2D>& vertices,
                                             const std::vector<double>& curvatures,
                                             const Adapters::ILogger* logger = nullptr);

 private:
  /**
   * Extract shape type from JSON
   */
  static std::string extractShapeType(const std::string& shapeJson);

  /**
   * Extract vertices array from JSON
   */
  static std::vector<Point2D> extractVertices(const std::string& shapeJson);

  /**
   * Extract radius from JSON (for Leaf shapes)
   */
  static double extractRadius(const std::string& shapeJson);

  /**
   * Extract curvatures array from JSON (for TriArc shapes)
   */
  static std::vector<double> extractCurvatures(const std::string& shapeJson);

  /**
   * Validate Leaf parameters
   */
  static void validateLeafParameters(const std::vector<Point2D>& vertices, double radius);

  /**
   * Validate TriArc parameters
   */
  static void validateTriArcParameters(const std::vector<Point2D>& vertices, const std::vector<double>& curvatures);
};

}  // namespace Geometry
}  // namespace ChipCarving

/**
 * ShapeFactory implementation
 */

#include "geometry/ShapeFactory.h"

#include <regex>
#include <sstream>
#include <stdexcept>

#include "adapters/IFusionInterface.h"
#include "geometry/Leaf.h"
#include "geometry/TriArc.h"

using ChipCarving::Geometry::distance;
using ChipCarving::Geometry::Leaf;
using ChipCarving::Geometry::Point2D;
using ChipCarving::Geometry::Shape;
using ChipCarving::Geometry::ShapeFactory;
using ChipCarving::Geometry::TriArc;

std::unique_ptr<Shape> ShapeFactory::createFromJson(const std::string& shapeJson, const Adapters::ILogger* logger) {
  std::string shapeType = extractShapeType(shapeJson);
  // Debug logging disabled for performance
  // if (logger) {
  //     logger->logDebug("ShapeFactory: Creating shape of type: " + shapeType);
  // }

  if (shapeType == "LEAF") {
    std::vector<Point2D> vertices = extractVertices(shapeJson);
    double radius = extractRadius(shapeJson);
    // Debug logging disabled for performance
    // if (logger) {
    //     logger->logDebug("ShapeFactory: Creating LEAF with " +
    //     std::to_string(vertices.size()) +
    //                      " vertices");
    // }
    return createLeaf(vertices, radius, logger);
  } else if (shapeType == "TRI_ARC") {
    std::vector<Point2D> vertices = extractVertices(shapeJson);
    std::vector<double> curvatures = extractCurvatures(shapeJson);
    // Debug logging disabled for performance
    // if (logger) {
    //     logger->logDebug("ShapeFactory: Creating TRI_ARC with " +
    //                      std::to_string(vertices.size()) + " vertices and " +
    //                      std::to_string(curvatures.size()) + " curvatures");
    // }
    return createTriArc(vertices, curvatures, logger);
  } else {
    throw std::runtime_error("Unknown shape type: " + shapeType);
  }
}

std::unique_ptr<Shape> ShapeFactory::createLeaf(const std::vector<Point2D>& vertices, double radius,
                                                const Adapters::ILogger* logger) {
  (void)logger;  // Suppress unused parameter warning
  validateLeafParameters(vertices, radius);
  return std::make_unique<Leaf>(vertices[0], vertices[1], radius);
}

std::unique_ptr<Shape> ShapeFactory::createTriArc(const std::vector<Point2D>& vertices,
                                                  const std::vector<double>& curvatures,
                                                  const Adapters::ILogger* logger) {
  (void)logger;  // Suppress unused parameter warning
  validateTriArcParameters(vertices, curvatures);

  // Convert curvatures to bulge factors (schema uses curvatures, TriArc uses
  // bulge factors)
  std::array<double, 3> bulgeFactors;
  for (size_t i = 0; i < 3; ++i) {
    bulgeFactors[i] = curvatures[i];  // Direct mapping for now
  }

  return std::make_unique<TriArc>(vertices[0], vertices[1], vertices[2], bulgeFactors);
}

std::string ShapeFactory::extractShapeType(const std::string& shapeJson) {
  // Extract "type": "LEAF" or "type": "TRI_ARC"
  std::regex typeRegex("\"type\"\\s*:\\s*\"([^\"]+)\"");
  std::smatch match;

  if (std::regex_search(shapeJson, match, typeRegex)) {
    return match[1].str();
  }

  throw std::runtime_error("No shape type found in JSON");
}

std::vector<Point2D> ShapeFactory::extractVertices(const std::string& shapeJson) {
  // Extract "vertices": [{"x": 1.0, "y": 2.0}, ...]
  std::regex verticesRegex("\"vertices\"\\s*:\\s*\\[([^\\]]+)\\]");
  std::smatch match;

  if (!std::regex_search(shapeJson, match, verticesRegex)) {
    throw std::runtime_error("No vertices found in JSON");
  }

  std::string verticesArray = match[1].str();
  std::vector<Point2D> vertices;

  // Extract individual points: {"x": 1.0, "y": 2.0}
  std::regex pointRegex("\\{\\s*\"x\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)\\s*,\\s*\"y\"\\s*:\\s*([-+]"
                        "?[0-9]*\\.?[0-9]+)\\s*\\}");
  std::sregex_iterator iter(verticesArray.begin(), verticesArray.end(), pointRegex);
  std::sregex_iterator end;

  for (; iter != end; ++iter) {
    double x = std::stod((*iter)[1].str());
    double y = std::stod((*iter)[2].str());
    vertices.emplace_back(x, y);
  }

  if (vertices.empty()) {
    throw std::runtime_error("No valid vertices found in JSON");
  }

  return vertices;
}

double ShapeFactory::extractRadius(const std::string& shapeJson) {
  // Extract "radius": 5.0
  std::regex radiusRegex("\"radius\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
  std::smatch match;

  if (std::regex_search(shapeJson, match, radiusRegex)) {
    return std::stod(match[1].str());
  }

  throw std::runtime_error("No radius found in JSON");
}

std::vector<double> ShapeFactory::extractCurvatures(const std::string& shapeJson) {
  // Extract "curvatures": [-0.125, -0.125, -0.125]
  std::regex curvaturesRegex("\"curvatures\"\\s*:\\s*\\[([^\\]]+)\\]");
  std::smatch match;

  if (!std::regex_search(shapeJson, match, curvaturesRegex)) {
    throw std::runtime_error("No curvatures found in JSON");
  }

  std::string curvaturesArray = match[1].str();
  std::vector<double> curvatures;

  // Extract individual numbers
  std::regex numberRegex("[-+]?[0-9]*\\.?[0-9]+");
  std::sregex_iterator iter(curvaturesArray.begin(), curvaturesArray.end(), numberRegex);
  std::sregex_iterator end;

  for (; iter != end; ++iter) {
    curvatures.push_back(std::stod(iter->str()));
  }

  if (curvatures.empty()) {
    throw std::runtime_error("No valid curvatures found in JSON");
  }

  return curvatures;
}

void ShapeFactory::validateLeafParameters(const std::vector<Point2D>& vertices, double radius) {
  if (vertices.size() != 2) {
    throw std::runtime_error("Leaf shape requires exactly 2 vertices, got " + std::to_string(vertices.size()));
  }

  if (radius <= 0.0) {
    throw std::runtime_error("Leaf radius must be positive, got " + std::to_string(radius));
  }

  // Check minimum radius requirement
  double chordLength = distance(vertices[0], vertices[1]);
  if (radius < chordLength / 2.0) {
    throw std::runtime_error("Leaf radius (" + std::to_string(radius) + ") must be at least half the chord length (" +
                             std::to_string(chordLength / 2.0) + ")");
  }
}

void ShapeFactory::validateTriArcParameters(const std::vector<Point2D>& vertices,
                                            const std::vector<double>& curvatures) {
  if (vertices.size() != 3) {
    throw std::runtime_error("TriArc shape requires exactly 3 vertices, got " + std::to_string(vertices.size()));
  }

  if (curvatures.size() != 3) {
    throw std::runtime_error("TriArc shape requires exactly 3 curvatures, got " + std::to_string(curvatures.size()));
  }

  // Check for degenerate triangle
  double area = std::abs((vertices[1].x - vertices[0].x) * (vertices[2].y - vertices[0].y) -
                         (vertices[2].x - vertices[0].x) * (vertices[1].y - vertices[0].y)) /
                2.0;
  if (area < 1e-9) {
    throw std::runtime_error("TriArc vertices are collinear (degenerate triangle)");
  }
}

/**
 * MedialAxisProcessor.h
 *
 * Class for computing medial axis of shapes using OpenVoronoi.
 * Handles coordinate transformation, OpenVoronoi computation, and result processing.
 */

#pragma once

#include <memory>
#include <vector>

#include "MedialAxisUtilities.h"
#include "Point2D.h"
#include "Shape.h"

namespace ChipCarving {
namespace Geometry {

/**
 * Coordinate transformation parameters for unit circle conversion
 */
struct TransformParams {
  Point2D offset{0, 0};       // Translation to center at origin
  double scale = 1.0;         // Uniform scale factor to fit in unit circle
  Point2D originalMin{0, 0};  // Original bounding box minimum
  Point2D originalMax{0, 0};  // Original bounding box maximum
};

/**
 * Complete medial axis computation results
 */
struct MedialAxisResults {
  std::vector<std::vector<Point2D>> chains{};       // Medial axis chains in world coordinates
  std::vector<std::vector<double>> clearanceRadii{};  // Clearance radii for each chain point
  TransformParams transform{};                      // Transform parameters used

  // Statistics
  int numChains = 0;
  int totalPoints = 0;
  double totalLength = 0.0;
  double minClearance = 0.0;
  double maxClearance = 0.0;

  // Success/error status
  bool success = false;
  std::string errorMessage{};
};

/**
 * MedialAxisProcessor - Encapsulates complete medial axis computation pipeline
 *
 * This class takes a Shape object and computes its medial axis using OpenVoronoi.
 * It handles all coordinate transformations, polygon approximation, and result processing.
 */
class MedialAxisProcessor {
 public:
  /**
   * Constructor with default parameters
   */
  MedialAxisProcessor();

  /**
   * Constructor with custom parameters
   * @param polygonTolerance Maximum error for polygon approximation (mm)
   * @param medialThreshold OpenVoronoi threshold for edge parallelism (0.0-1.0)
   */
  MedialAxisProcessor(double polygonTolerance, double medialThreshold);

  /**
   * Compute medial axis for a shape
   * @param shape The shape to process
   * @return Complete medial axis results with chains and clearance radii
   */
  MedialAxisResults computeMedialAxis(const Shape& shape);

  /**
   * Compute medial axis from pre-polygonized vertices
   * @param polygon The polygon vertices in world coordinates
   * @return Complete medial axis results
   */
  MedialAxisResults computeMedialAxis(const std::vector<Point2D>& polygon);

  /**
   * Get sampled medial axis paths suitable for toolpath generation
   * @param results MedialAxisResults from computeMedialAxis
   * @param spacing Spacing between sampled points (mm)
   * @return Vector of sampled paths with uniform point spacing
   */
  std::vector<SampledMedialPath> getSampledPaths(const MedialAxisResults& results, double spacing = 1.0);

  // Parameter accessors
  double getPolygonTolerance() const {
    return polygonTolerance_;
  }
  void setPolygonTolerance(double tolerance) {
    polygonTolerance_ = tolerance;
  }

  double getMedialThreshold() const {
    return medialThreshold_;
  }
  void setMedialThreshold(double threshold) {
    medialThreshold_ = threshold;
  }

  // Enable/disable verbose logging for debugging
  void setVerbose(bool verbose) {
    verbose_ = verbose;
  }

  // Set MedialAxisWalk intermediate points parameter (for testing)
  void setMedialAxisWalkPoints(int points) {
    medialAxisWalkPoints_ = points;
  }
  int getMedialAxisWalkPoints() const {
    return medialAxisWalkPoints_;
  }

 private:
  double polygonTolerance_;   // Maximum error for polygon approximation (mm)
  double medialThreshold_;    // OpenVoronoi threshold for edge parallelism
  bool verbose_;              // Enable verbose logging
  int medialAxisWalkPoints_;  // MedialAxisWalk intermediate points parameter

  /**
   * Transform polygon from world coordinates to unit circle
   * @param polygon Input polygon in world coordinates
   * @param params Output transformation parameters
   * @return Transformed polygon in unit circle coordinates
   */
  std::vector<Point2D> transformToUnitCircle(const std::vector<Point2D>& polygon, TransformParams& params);

  /**
   * Transform point from unit circle back to world coordinates
   * @param unitPoint Point in unit circle coordinates
   * @param params Transformation parameters from transformToUnitCircle
   * @return Point in world coordinates
   */
  Point2D transformFromUnitCircle(const Point2D& unitPoint, const TransformParams& params);

  /**
   * Validate polygon for OpenVoronoi processing
   * @param polygon Polygon in unit circle coordinates
   * @return true if polygon is valid for OpenVoronoi
   */
  bool validatePolygonForOpenVoronoi(const std::vector<Point2D>& polygon);

  /**
   * Core OpenVoronoi computation on unit circle polygon
   * @param transformedPolygon Polygon in unit circle coordinates
   * @param results Output results structure (will be populated)
   * @return true if computation succeeded
   */
  bool computeOpenVoronoi(const std::vector<Point2D>& transformedPolygon, MedialAxisResults& results);

  /**
   * Log message if verbose mode is enabled
   */
  void log(const std::string& message);
};

}  // namespace Geometry
}  // namespace ChipCarving

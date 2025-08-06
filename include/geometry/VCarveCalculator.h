/**
 * VCarveCalculator.h
 *
 * Calculator for generating V-carve toolpaths from medial axis data.
 * Converts sampled medial axis paths with clearance radii into 3D V-carve toolpaths.
 */

#pragma once

#include <functional>

#include "../../src/adapters/IFusionInterface.h"
#include "MedialAxisProcessor.h"
#include "MedialAxisUtilities.h"
#include "VCarvePath.h"

namespace ChipCarving {
namespace Geometry {

/**
 * VCarveCalculator - Converts medial axis data to V-carve toolpaths
 *
 * This class takes sampled medial axis paths and generates 3D V-carve toolpaths
 * by calculating appropriate Z-depths based on tool geometry and clearance radii.
 */
class VCarveCalculator {
 public:
  /**
   * Default constructor
   */
  VCarveCalculator();

  /**
   * Generate V-carve toolpaths from medial axis results
   * @param medialResults Complete medial axis computation results
   * @param params Tool and V-carve parameters
   * @return V-carve toolpaths ready for 3D sketch generation
   */
  VCarveResults generateVCarvePaths(const MedialAxisResults& medialResults,
                                    const Adapters::MedialAxisParameters& params);

  /**
   * Generate V-carve toolpaths from sampled medial paths
   * @param sampledPaths Pre-sampled medial axis paths
   * @param params Tool and V-carve parameters
   * @return V-carve toolpaths ready for 3D sketch generation
   */
  VCarveResults generateVCarvePaths(const std::vector<SampledMedialPath>& sampledPaths,
                                    const Adapters::MedialAxisParameters& params);

  /**
   * Function type for querying surface Z at XY location
   * @param x X coordinate in mm
   * @param y Y coordinate in mm
   * @return Z coordinate at surface, or NaN if no surface
   */
  using SurfaceQueryFunction = std::function<double(double x, double y)>;

  /**
   * Generate V-carve toolpaths with surface projection
   * @param sampledPaths Pre-sampled medial axis paths
   * @param params Tool and V-carve parameters
   * @param sketchPlaneZ Z position of sketch plane in mm
   * @param surfaceQuery Function to query surface Z at XY position
   * @return V-carve toolpaths projected onto surface
   */
  VCarveResults generateVCarvePathsWithSurface(const std::vector<SampledMedialPath>& sampledPaths,
                                               const Adapters::MedialAxisParameters& params, double sketchPlaneZ,
                                               SurfaceQueryFunction surfaceQuery);

  /**
   * Calculate V-carve depth for a given clearance radius and tool
   * @param clearanceRadius Clearance radius from medial axis (mm)
   * @param toolAngle V-bit angle in degrees
   * @param maxDepth Maximum allowed depth (safety limit)
   * @return Calculated depth (mm, positive = below sketch plane)
   */
  static double calculateVCarveDepth(double clearanceRadius, double toolAngle, double maxDepth);

 private:
  /**
   * Convert a single sampled medial path to V-carve path
   * @param sampledPath Input medial axis path with clearances
   * @param params Tool and V-carve parameters
   * @return Generated V-carve path
   */
  VCarvePath convertSampledPath(const SampledMedialPath& sampledPath, const Adapters::MedialAxisParameters& params);

  /**
   * Validate tool parameters for V-carve generation
   * @param params Parameters to validate
   * @return true if parameters are valid
   */
  bool validateParameters(const Adapters::MedialAxisParameters& params);

  /**
   * Apply path optimization and merging
   * @param paths Input paths to optimize
   * @param params Parameters for optimization
   * @return Optimized paths
   */
  std::vector<VCarvePath> optimizePaths(const std::vector<VCarvePath>& paths,
                                        const Adapters::MedialAxisParameters& params);

  /**
   * Check if two path endpoints can be connected
   * @param path1 First path
   * @param path2 Second path
   * @param tolerance Connection tolerance in mm
   * @return true if paths can be merged
   */
  bool canConnectPaths(const VCarvePath& path1, const VCarvePath& path2, double tolerance = 0.1);

  /**
   * Merge two connectable paths into one
   * @param path1 First path
   * @param path2 Second path
   * @return Merged path
   */
  VCarvePath mergePaths(const VCarvePath& path1, const VCarvePath& path2);
};

}  // namespace Geometry
}  // namespace ChipCarving

/**
 * VCarveCalculatorSurface.cpp
 *
 * Surface projection functionality for V-carve calculations.
 * Split from VCarveCalculator.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <set>

#include "../../include/geometry/VCarveCalculator.h"
#include "../../include/utils/logging.h"

namespace ChipCarving {
namespace Geometry {

VCarveResults VCarveCalculator::generateVCarvePathsWithSurface(
    const std::vector<SampledMedialPath>& sampledPaths,
    const Adapters::MedialAxisParameters& params, double sketchPlaneZ,
    SurfaceQueryFunction surfaceQuery) {
  VCarveResults results;

  // Validate inputs
  if (!validateParameters(params)) {
    results.errorMessage = "Invalid V-carve parameters";
    return results;
  }

  if (sampledPaths.empty()) {
    results.errorMessage = "No sampled paths provided";
    return results;
  }

  try {
    // Convert each sampled path to V-carve path with surface projection
    std::vector<VCarvePath> vcarvePathsRaw;

    for (const auto& sampledPath : sampledPaths) {
      if (sampledPath.points.empty()) {
        continue;  // Skip empty paths
      }

      VCarvePath vcarvePath;

      // Convert each sampled point to V-carve point with surface-adjusted depth
      for (const auto& sampledPoint : sampledPath.points) {
        // Calculate base V-carve depth from clearance radius
        double baseDepth =
            calculateVCarveDepth(sampledPoint.clearanceRadius, params.toolAngle,
                                 params.maxVCarveDepth);

        // Get position in mm
        Point2D positionMm = sampledPoint.position;

        // Query surface Z at this XY position (units: cm for Fusion API)
        double surfaceZ = surfaceQuery(
            positionMm.x / 10.0, positionMm.y / 10.0);  // Convert mm to cm

        // Calculate final depth based on surface projection
        double finalDepth;
        if (params.projectToSurface && !std::isnan(surfaceZ)) {
          // Surface found - calculate proper depth
          // REVISED APPROACH - Much simpler:
          // - surfaceZ is the Z coordinate of the surface (in cm)
          // - baseDepth is the desired cutting depth below surface (in cm)
          // - finalDepth should be in the VCarvePoint coordinate system (cm)

          // For surface projection, we want to cut baseDepth below the surface
          // So the final depth is simply baseDepth (the same as non-surface
          // case) The surface projection will be handled by the sketch creation
          // later
          finalDepth = baseDepth;

          // Debug logging
          static bool loggedOnce = false;
          if (!loggedOnce) {
            LOG_DEBUG("=== V-CARVE DEPTH CALCULATION DEBUG ===");
            LOG_DEBUG("  XY position (mm): (" << positionMm.x << ", "
                                              << positionMm.y << ")");
            LOG_DEBUG("  XY position (cm): (" << positionMm.x / 10.0 << ", "
                                              << positionMm.y / 10.0 << ")");
            LOG_DEBUG("  surfaceZ (cm): " << surfaceZ);
            LOG_DEBUG("  baseDepth (cm): " << baseDepth);
            LOG_DEBUG("  sketchPlaneZ (cm): " << sketchPlaneZ);
            LOG_DEBUG("  finalDepth (cm): " << finalDepth);
            LOG_DEBUG(
                "  clearanceRadius (mm): " << sampledPoint.clearanceRadius);
            loggedOnce = true;
          }
        } else {
          // No surface or projection disabled - use base depth
          finalDepth = baseDepth;
        }

        // Create V-carve point with adjusted depth
        VCarvePoint vcarvePoint(positionMm, finalDepth,
                                sampledPoint.clearanceRadius);
        vcarvePath.points.push_back(vcarvePoint);
      }

      // Update path properties
      vcarvePath.totalLength = vcarvePath.calculateLength();
      vcarvePath.isClosed = false;  // For now, treat all paths as open

      if (vcarvePath.isValid()) {
        vcarvePathsRaw.push_back(vcarvePath);
      }
    }

    if (vcarvePathsRaw.empty()) {
      results.errorMessage = "No valid V-carve paths generated";
      return results;
    }

    // Apply path optimization and merging
    results.paths = optimizePaths(vcarvePathsRaw, params);

    // Update statistics
    results.updateStatistics();
    results.success = true;
  } catch (const std::exception& e) {
    results.errorMessage =
        "Exception during V-carve generation: " + std::string(e.what());
    results.success = false;
  }

  return results;
}

}  // namespace Geometry
}  // namespace ChipCarving

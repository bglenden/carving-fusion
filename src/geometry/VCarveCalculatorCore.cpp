/**
 * VCarveCalculatorCore.cpp
 *
 * Core V-carve calculation functionality - constructors and main generation
 * methods. Split from VCarveCalculator.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <set>

#include "../../include/geometry/VCarveCalculator.h"

namespace ChipCarving {
namespace Geometry {

VCarveCalculator::VCarveCalculator() {}

VCarveResults VCarveCalculator::generateVCarvePaths(
    const MedialAxisResults& medialResults,
    const Adapters::MedialAxisParameters& params) {
  VCarveResults results;

  // Validate inputs
  if (!validateParameters(params)) {
    results.errorMessage = "Invalid V-carve parameters";
    return results;
  }

  if (!medialResults.success || medialResults.chains.empty()) {
    results.errorMessage = "Invalid or empty medial axis results";
    return results;
  }

  try {
    // Convert each medial axis chain directly to V-carve path
    // This preserves the exact vertices from OpenVoronoi without additional
    // interpolation
    std::vector<VCarvePath> vcarvePathsRaw;

    for (size_t i = 0; i < medialResults.chains.size(); ++i) {
      const auto& chain = medialResults.chains[i];
      const auto& clearances = medialResults.clearanceRadii[i];

      if (chain.empty() || chain.size() != clearances.size()) {
        continue;  // Skip invalid chains
      }

      VCarvePath vcarvePath;

      // Convert each point in the chain
      for (size_t j = 0; j < chain.size(); ++j) {
        // Calculate V-carve depth using the exact clearance radius from
        // OpenVoronoi clearances[j] is in cm, convert to mm for depth
        // calculation
        double clearanceMm = clearances[j] * 10.0;
        double depth = calculateVCarveDepth(clearanceMm, params.toolAngle,
                                            params.maxVCarveDepth);

        // Create V-carve point - chain points are already in world coordinates
        // (cm) Convert to mm for consistency with the rest of the system
        Point2D positionMm(chain[j].x * 10.0, chain[j].y * 10.0);

        VCarvePoint vcarvePoint(positionMm, depth, clearanceMm);
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

VCarveResults VCarveCalculator::generateVCarvePaths(
    const std::vector<SampledMedialPath>& sampledPaths,
    const Adapters::MedialAxisParameters& params) {
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
    // Convert each sampled path to V-carve path
    std::vector<VCarvePath> vcarvePathsRaw;

    for (const auto& sampledPath : sampledPaths) {
      if (sampledPath.points.empty()) {
        continue;  // Skip empty paths
      }

      VCarvePath vcarvePath = convertSampledPath(sampledPath, params);
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

double VCarveCalculator::calculateVCarveDepth(double clearanceRadius,
                                              double toolAngle,
                                              double maxDepth) {
  if (clearanceRadius <= 0.0 || toolAngle <= 0.0 || toolAngle >= 180.0) {
    return 0.0;
  }

  // Convert angle to radians and use half-angle for V-bit geometry
  double halfAngleRad = (toolAngle * M_PI / 180.0) / 2.0;

  // Calculate depth using trigonometry: depth = radius / tan(half_angle)
  double depth = clearanceRadius / std::tan(halfAngleRad);

  // Apply safety limit
  return std::min(depth, maxDepth);
}

bool VCarveCalculator::validateParameters(
    const Adapters::MedialAxisParameters& params) {
  // Check tool angle is reasonable
  if (params.toolAngle <= 0.0 || params.toolAngle >= 180.0) {
    return false;
  }

  // Check maximum depth is positive
  if (params.maxVCarveDepth <= 0.0) {
    return false;
  }

  // Check sampling distance is positive
  if (params.samplingDistance <= 0.0) {
    return false;
  }

  return true;
}

}  // namespace Geometry
}  // namespace ChipCarving
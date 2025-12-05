/**
 * FusionWorkspaceProfile.cpp
 *
 * Profile and geometry extraction operations for FusionWorkspace
 * Split from FusionWorkspaceProfile.cpp for maintainability
 */

#include <cmath>
#include <string>
#include <vector>

#include "FusionAPIAdapter.h"
#include "FusionWorkspaceProfileTypes.h"
#include "utils/logging.h"

using adsk::core::Ptr;

namespace ChipCarving {
namespace Adapters {

bool FusionWorkspace::extractProfileVertices(const std::string& entityId,
                                             std::vector<std::pair<double, double>>& vertices,
                                             TransformParams& transform) {
  // Enhanced UI Phase 5.2: Extract geometry from Fusion 360 sketch profiles
  // FIXED: This function now returns vertices in WORLD COORDINATES
  // for proper medial axis computation. Construction geometry is created
  // on the same sketch plane, so coordinate alignment works correctly.
  vertices.clear();

  // Use centralized debug logging
  LOG_DEBUG("=== PROFILE EXTRACTION === extractProfileVertices called for: " << entityId);

  // Use extracted profile search method
  Ptr<adsk::fusion::Profile> profile = findProfileByEntityToken(entityId);
  if (!profile) {
    LOG_ERROR("Could not find selected profile with entityId: " << entityId);
    return false;
  }

  LOG_DEBUG("Processing selected profile");

  // Extract curves from profile using extracted method
  std::vector<CurveData> allCurves;
  if (!extractCurvesFromProfile(profile, allCurves, transform)) {
    LOG_ERROR("Failed to extract curves from profile");
    return false;
  }

  // Now chain the curves in the correct order to form a closed polygon
  LOG_DEBUG("Starting curve chaining algorithm...");

  std::vector<size_t> chainOrder;
  const double tolerance = 0.001;  // 0.01mm tolerance for point matching

  // Start with the first curve
  chainOrder.push_back(0);
  allCurves[0].used = true;

  Ptr<adsk::core::Point3D> currentEndPoint = allCurves[0].endPoint;
  LOG_DEBUG("Starting chain with curve 0, end point: (" << currentEndPoint->x() << ", " << currentEndPoint->y() << ")");

  // Find the next connected curve
  for (size_t chainPos = 1; chainPos < allCurves.size(); ++chainPos) {
    bool foundNext = false;

    for (size_t i = 0; i < allCurves.size(); ++i) {
      if (allCurves[i].used)
        continue;

      // Check if this curve's start connects to our current end
      double distToStart = std::sqrt(std::pow(allCurves[i].startPoint->x() - currentEndPoint->x(), 2) +
                                     std::pow(allCurves[i].startPoint->y() - currentEndPoint->y(), 2));

      // Check if this curve's end connects to our current end (reverse
      // direction)
      double distToEnd = std::sqrt(std::pow(allCurves[i].endPoint->x() - currentEndPoint->x(), 2) +
                                   std::pow(allCurves[i].endPoint->y() - currentEndPoint->y(), 2));

      if (distToStart < tolerance) {
        // Normal direction connection
        chainOrder.push_back(i);
        allCurves[i].used = true;
        currentEndPoint = allCurves[i].endPoint;
        foundNext = true;
        LOG_DEBUG("Chained curve " << i << " (normal), end point: (" << currentEndPoint->x() << ", "
                                   << currentEndPoint->y() << ")");
        break;
      }
      if (distToEnd < tolerance) {
        // Reverse direction connection - need to reverse the stroke points
        chainOrder.push_back(i | 0x80000000);  // Mark as reversed with high bit
        allCurves[i].used = true;
        currentEndPoint = allCurves[i].startPoint;
        foundNext = true;
        LOG_DEBUG("Chained curve " << i << " (REVERSED), end point: (" << currentEndPoint->x() << ", "
                                   << currentEndPoint->y() << ")");
        break;
      }
    }

    if (!foundNext) {
      LOG_WARNING("Could not find next curve to chain at position " << chainPos);
      break;
    }
  }

  std::string chainOrderStr = "Chaining complete. Order: ";
  for (size_t idx : chainOrder) {
    bool reversed = (idx & 0x80000000) != 0;
    size_t curveIdx = idx & 0x7FFFFFFF;
    chainOrderStr += std::to_string(curveIdx) + (reversed ? "R" : "") + " ";
  }
  LOG_DEBUG(chainOrderStr);

  // Now build the final vertex list from the chained curves
  for (size_t i = 0; i < chainOrder.size(); ++i) {
    bool reversed = (chainOrder[i] & 0x80000000) != 0;
    size_t curveIdx = chainOrder[i] & 0x7FFFFFFF;

    const auto& curveData = allCurves[curveIdx];
    const auto& strokePoints = curveData.strokePoints;

    LOG_DEBUG("Adding curve " << curveIdx << (reversed ? " (reversed)" : "") << " with " << strokePoints.size()
                              << " points");

    // Determine how many points to add (always skip last point to avoid
    // duplicates) For closed polygons, we don't want to duplicate the final
    // vertex
    size_t numPoints = strokePoints.size() - 1;

    if (reversed) {
      // Add points in reverse order, skip first point (which is last in
      // reverse)
      for (int j = strokePoints.size() - 1; j >= 1; --j) {
        if (strokePoints[j]) {
          double x = strokePoints[j]->x();
          double y = strokePoints[j]->y();
          double z = strokePoints[j]->z();

          // NOTE: With world coordinates, Z can be non-zero (expected)
          // This is the correct behavior for medial axis computation
          if (std::abs(z) > 0.001) {
            LOG_DEBUG("Point has Z value: " << z << " cm (world coordinates)");
          }

          vertices.push_back({x, y});
        }
      }
    } else {
      // Add points in normal order
      for (size_t j = 0; j < numPoints; ++j) {
        if (strokePoints[j]) {
          double x = strokePoints[j]->x();
          double y = strokePoints[j]->y();
          double z = strokePoints[j]->z();

          // NOTE: With world coordinates, Z can be non-zero (expected)
          // This is the correct behavior for medial axis computation
          if (std::abs(z) > 0.001) {
            LOG_DEBUG("Point has Z value: " << z << " cm (world coordinates)");
          }

          vertices.push_back({x, y});
        }
      }
    }
  }

  LOG_DEBUG("Final chained polygon has " << vertices.size() << " vertices");

  if (vertices.empty()) {
    LOG_ERROR("No vertices in final chained polygon");
    return false;
  }

  // DO NOT transform vertices here - MedialAxisProcessor will handle all
  // transformations Just store dummy transform parameters since they're
  // required by the interface
  transform.centerX = 0.0;
  transform.centerY = 0.0;
  transform.scale = 1.0;

  LOG_DEBUG("Extracted " << vertices.size() << " vertices from real profile (in world coordinates cm)");

  return true;
}

// NOTE: extractProfileGeometry() is in FusionWorkspaceProfileGeometry.cpp

}  // namespace Adapters
}  // namespace ChipCarving

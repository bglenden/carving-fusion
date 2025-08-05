/**
 * PluginManagerPathsGeometry.cpp
 *
 * Profile geometry extraction logic for PluginManager
 * Split from PluginManagerPaths.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>

#include "../../include/geometry/Point2D.h"
#include "../../include/geometry/Point3D.h"
#include "../../include/geometry/VCarveCalculator.h"
#include "../utils/UnitConversion.h"
#include "PluginManager.h"

namespace ChipCarving {
namespace Core {

bool PluginManager::extractProfileGeometry(
    const Adapters::SketchSelection& selection,
    std::vector<std::vector<Geometry::Point2D>>& profilePolygons,
    std::vector<Adapters::IWorkspace::TransformParams>& profileTransforms) {
  if (!initialized_ || !workspace_) {
    return false;
  }

  if (!selection.isValid || selection.selectedEntityIds.empty()) {
    LOG_INFO("Invalid selection or no entity IDs. isValid="
             << selection.isValid
             << ", entityIds=" << selection.selectedEntityIds.size());
    return false;
  }

  LOG_INFO("Extracting profile geometry for "
           << selection.selectedEntityIds.size() << " profiles");

  profilePolygons.clear();
  profilePolygons.reserve(selection.selectedEntityIds.size());
  profileTransforms.clear();
  profileTransforms.reserve(selection.selectedEntityIds.size());

  try {
    // IMMEDIATE EXTRACTION APPROACH: Use cached vertices directly
    if (!selection.selectedProfiles.empty()) {
      LOG_INFO("Using immediate extraction approach with "
               << selection.selectedProfiles.size() << " cached profiles");

      // Use the cached geometry that was extracted immediately when selections
      // were made
      for (size_t i = 0; i < selection.selectedProfiles.size(); ++i) {
        const auto& profileGeom = selection.selectedProfiles[i];

        if (profileGeom.vertices.size() < 3) {
          LOG_INFO("Profile " << i << " has insufficient vertices ("
                              << profileGeom.vertices.size() << "), skipping");
          continue;
        }

        LOG_INFO("Using cached geometry for profile "
                 << i << " from sketch '" << profileGeom.sketchName << "' with "
                 << profileGeom.vertices.size() << " vertices");

        // Convert cached vertices to Point2D vector
        std::vector<Geometry::Point2D> polygon;
        polygon.reserve(profileGeom.vertices.size());

        std::transform(profileGeom.vertices.begin(), profileGeom.vertices.end(),
                       std::back_inserter(polygon), [](const auto& vertex) {
                         return Geometry::Point2D(vertex.first, vertex.second);
                       });

        // Log the converted polygon for debugging
        LOG_INFO("Converted cached vertices to Point2D polygon with "
                 << polygon.size() << " points");
        if (!polygon.empty()) {
          // Log first few points
          size_t numToLog = std::min(size_t(4), polygon.size());
          for (size_t p = 0; p < numToLog; ++p) {
            LOG_INFO("  Point " << p << ": (" << polygon[p].x << ", "
                                << polygon[p].y << ")");
          }
        }

        profilePolygons.push_back(std::move(polygon));
        profileTransforms.push_back(profileGeom.transform);
      }
    } else {
      // Fallback for old callers that don't populate selectedProfiles
      for (const auto& entityId : selection.selectedEntityIds) {
        std::vector<std::pair<double, double>> rawVertices;
        Adapters::IWorkspace::TransformParams transform;

        // Extract vertices from the profile using workspace interface
        bool extractionSuccess = workspace_->extractProfileVertices(
            entityId, rawVertices, transform);

        if (!extractionSuccess || rawVertices.empty()) {
          continue;  // Skip this profile but continue with others
        }

        // Convert raw vertices to Point2D vector
        std::vector<Geometry::Point2D> polygon;
        polygon.reserve(rawVertices.size());

        // Log first few vertices for debugging
        for (size_t i = 0; i < std::min(size_t(5), rawVertices.size()); ++i) {
        }

        // Calculate and log bounds
        if (!rawVertices.empty()) {
          double minX = rawVertices[0].first, maxX = rawVertices[0].first;
          double minY = rawVertices[0].second, maxY = rawVertices[0].second;
          for (const auto& v : rawVertices) {
            minX = std::min(minX, v.first);
            maxX = std::max(maxX, v.first);
            minY = std::min(minY, v.second);
            maxY = std::max(maxY, v.second);
          }
        }

        std::transform(rawVertices.begin(), rawVertices.end(),
                       std::back_inserter(polygon), [](const auto& vertex) {
                         return Geometry::Point2D(vertex.first, vertex.second);
                       });

        // Validate polygon has minimum vertices for processing
        if (polygon.size() < 3) {
          continue;
        }

        // Follow Fusion's convention: polygons are implicitly closed
        // Do NOT add duplicate closing vertex - this causes validation issues
        profilePolygons.push_back(std::move(polygon));
        profileTransforms.push_back(
            transform);  // Store the transformation parameters
      }
    }

    if (profilePolygons.empty()) {
      LOG_INFO("No valid profile polygons extracted - returning false");
      return false;
    }

    LOG_INFO("Successfully extracted " << profilePolygons.size()
                                       << " profile polygons");
    for (size_t i = 0; i < profilePolygons.size(); ++i) {
      LOG_INFO("  Profile " << i << ": " << profilePolygons[i].size()
                            << " vertices");
    }

    return true;
  } catch (const std::exception& e) {
    return false;
  } catch (...) {
    return false;
  }
}

}  // namespace Core
}  // namespace ChipCarving

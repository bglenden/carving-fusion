/**
 * PluginManagerPathsGeometry.cpp
 *
 * Profile geometry extraction logic for PluginManager
 * Split from PluginManagerPaths.cpp for maintainability
 */

#include <algorithm>
#include <vector>

#include "core/PluginManager.h"
#include "geometry/Point2D.h"
#include "utils/logging.h"

namespace ChipCarving {
namespace Core {

namespace {

// Convert vertex pairs to Point2D polygon
std::vector<Geometry::Point2D> convertToPolygon(
    const std::vector<std::pair<double, double>>& vertices) {
  std::vector<Geometry::Point2D> polygon;
  polygon.reserve(vertices.size());
  std::transform(vertices.begin(), vertices.end(), std::back_inserter(polygon),
                 [](const auto& v) { return Geometry::Point2D(v.first, v.second); });
  return polygon;
}

// Extract from cached profile geometry
void extractFromCachedProfiles(
    const std::vector<Adapters::ProfileGeometry>& profiles,
    std::vector<std::vector<Geometry::Point2D>>& polygons,
    std::vector<Adapters::IWorkspace::TransformParams>& transforms) {
  for (size_t i = 0; i < profiles.size(); ++i) {
    const auto& profileGeom = profiles[i];

    if (profileGeom.vertices.size() < 3) {
      LOG_INFO("Profile " << i << " has insufficient vertices, skipping");
      continue;
    }

    LOG_INFO("Using cached geometry for profile " << i << " from sketch '"
             << profileGeom.sketchName << "' with " << profileGeom.vertices.size() << " vertices");

    auto polygon = convertToPolygon(profileGeom.vertices);
    polygons.push_back(std::move(polygon));
    transforms.push_back(profileGeom.transform);
  }
}

// Extract via workspace interface (fallback path)
void extractFromEntityIds(
    const std::vector<std::string>& entityIds,
    Adapters::IWorkspace* workspace,
    std::vector<std::vector<Geometry::Point2D>>& polygons,
    std::vector<Adapters::IWorkspace::TransformParams>& transforms) {
  for (const auto& entityId : entityIds) {
    std::vector<std::pair<double, double>> rawVertices;
    Adapters::IWorkspace::TransformParams transform;

    if (!workspace->extractProfileVertices(entityId, rawVertices, transform)) {
      continue;
    }

    if (rawVertices.size() < 3) {
      continue;
    }

    auto polygon = convertToPolygon(rawVertices);
    polygons.push_back(std::move(polygon));
    transforms.push_back(transform);
  }
}

}  // namespace

bool PluginManager::extractProfileGeometry(
    const Adapters::SketchSelection& selection,
    std::vector<std::vector<Geometry::Point2D>>& profilePolygons,
    std::vector<Adapters::IWorkspace::TransformParams>& profileTransforms) {
  if (!initialized_ || !workspace_) {
    return false;
  }

  if (!selection.isValid || selection.selectedEntityIds.empty()) {
    LOG_INFO("Invalid selection or no entity IDs");
    return false;
  }

  LOG_INFO("Extracting profile geometry for " << selection.selectedEntityIds.size() << " profiles");

  profilePolygons.clear();
  profileTransforms.clear();

  try {
    if (!selection.selectedProfiles.empty()) {
      extractFromCachedProfiles(selection.selectedProfiles, profilePolygons, profileTransforms);
    } else {
      extractFromEntityIds(selection.selectedEntityIds, workspace_.get(),
                           profilePolygons, profileTransforms);
    }

    if (profilePolygons.empty()) {
      LOG_INFO("No valid profile polygons extracted");
      return false;
    }

    LOG_INFO("Successfully extracted " << profilePolygons.size() << " profile polygons");
    return true;
  } catch (const std::exception& e) {
    return false;
  } catch (...) {
    return false;
  }
}

}  // namespace Core
}  // namespace ChipCarving

/**
 * PluginManagerPathsGeometry.cpp
 *
 * Profile geometry extraction logic for PluginManager
 * Split from PluginManagerPaths.cpp for maintainability
 */

#include "PluginManager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <set>
#include <sstream>

#include "../../include/geometry/Point2D.h"
#include "../../include/geometry/Point3D.h"
#include "../../include/geometry/VCarveCalculator.h"
#include "../../include/utils/TempFileManager.h"
#include "../utils/UnitConversion.h"

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
        return false;
    }

    profilePolygons.clear();
    profilePolygons.reserve(selection.selectedEntityIds.size());
    profileTransforms.clear();
    profileTransforms.reserve(selection.selectedEntityIds.size());

    try {
        for (const auto& entityId : selection.selectedEntityIds) {
            std::vector<std::pair<double, double>> rawVertices;
            Adapters::IWorkspace::TransformParams transform;

            // Extract vertices from the profile using workspace interface
            bool extractionSuccess =
                workspace_->extractProfileVertices(entityId, rawVertices, transform);

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

            std::transform(rawVertices.begin(), rawVertices.end(), std::back_inserter(polygon),
                [](const auto& vertex) {
                    return Geometry::Point2D(vertex.first, vertex.second);
                });

            // Validate polygon has minimum vertices for processing
            if (polygon.size() < 3) {
                continue;
            }

            // Follow Fusion's convention: polygons are implicitly closed
            // Do NOT add duplicate closing vertex - this causes validation issues

            profilePolygons.push_back(std::move(polygon));
            profileTransforms.push_back(transform);  // Store the transformation parameters
        }

        if (profilePolygons.empty()) {
            return false;
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

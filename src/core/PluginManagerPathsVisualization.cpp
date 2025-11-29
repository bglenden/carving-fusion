/**
 * PluginManagerPathsVisualization.cpp
 *
 * Construction geometry visualization logic for PluginManager
 * Split from PluginManagerPaths.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>

#include "geometry/Point2D.h"
#include "geometry/Point3D.h"
#include "geometry/VCarveCalculator.h"
#include "utils/UnitConversion.h"
#include "PluginManager.h"

namespace ChipCarving {
namespace Core {

void PluginManager::addConstructionGeometryVisualization(Adapters::ISketch* sketch,
                                                         const Geometry::MedialAxisResults& results,
                                                         const Adapters::MedialAxisParameters& params,
                                                         const Adapters::IWorkspace::TransformParams& /* transform */,
                                                         const std::vector<Geometry::Point2D>& polygon) {
  if (!sketch || !results.success) {
    return;
  }

  try {
    // NOTE: Not applying any coordinate transformations
    // Fusion handles the transformation when creating sketch entities on the
    // correct plane

    // Get properly sampled medial axis paths at user-specified sampling
    // distance
    auto sampledPaths = medialProcessor_->getSampledPaths(results, params.samplingDistance);

    // Enhanced UI Phase 5.3: Add medial axis lines using sampled paths
    if (params.showMedialLines && !sampledPaths.empty()) {
      size_t totalLinesDrawn = 0;
      const size_t MAX_CONSTRUCTION_LINES = 1000;  // Safety limit

      for (size_t pathIdx = 0; pathIdx < sampledPaths.size(); ++pathIdx) {
        const auto& path = sampledPaths[pathIdx];

        // Draw lines connecting points in the sampled path
        for (size_t i = 0; i < path.points.size() - 1; ++i) {
          if (totalLinesDrawn >= MAX_CONSTRUCTION_LINES) {
            break;
          }
          const auto& p1 = path.points[i].position;
          const auto& p2 = path.points[i + 1].position;

          // Sampled paths are already in world coordinates (mm)
          double x1_world_mm = p1.x;
          double y1_world_mm = p1.y;
          double x2_world_mm = p2.x;
          double y2_world_mm = p2.y;

          // NOTE: Temporarily removing transformation to test
          // The coordinates from extractProfileVertices might already be in the
          // correct space

          // Debug logging for first few lines
          if (totalLinesDrawn < 5) {
            // TODO(developer): Add implementation
          }

          bool success = sketch->addConstructionLine(x1_world_mm, y1_world_mm, x2_world_mm, y2_world_mm);
          if (!success) {
            // TODO(developer): Add implementation
          }
          totalLinesDrawn++;
        }
        if (totalLinesDrawn >= MAX_CONSTRUCTION_LINES)
          break;
      }
    }

    // Enhanced UI Phase 5.3: Add clearance circles at actual medial axis
    // vertices only
    if (params.showClearanceCircles && results.success && !results.chains.empty()) {
      size_t totalCirclesDrawn = 0;
      const size_t MAX_CONSTRUCTION_CIRCLES = 500;  // Safety limit

      // Draw clearance circles only at actual medial axis vertices from
      // OpenVoronoi
      for (size_t chainIdx = 0; chainIdx < results.chains.size(); ++chainIdx) {
        const auto& chain = results.chains[chainIdx];
        const auto& clearances = results.clearanceRadii[chainIdx];

        if (chain.size() != clearances.size()) {
          continue;
        }

        // Draw clearance circles for ALL vertices exactly as OpenVoronoi
        // generated them This confirms every circle shown is a genuine
        // OpenVoronoi medial axis vertex

        for (size_t i = 0; i < chain.size(); ++i) {
          if (totalCirclesDrawn >= MAX_CONSTRUCTION_CIRCLES) {
            break;
          }

          // Chain points are in world coordinates (cm), convert to mm
          double x_world_mm = chain[i].x * 10.0;
          double y_world_mm = chain[i].y * 10.0;
          double radius_world_mm = clearances[i] * 10.0;

          // Log every circle to verify they're all from OpenVoronoi

          // Draw clearance circle (even for very small radii to show boundary
          // points)
          bool circleSuccess = true;
          if (radius_world_mm >= 0.01) {  // Only draw circle if radius is visible
            circleSuccess = sketch->addConstructionCircle(x_world_mm, y_world_mm, radius_world_mm);
          }

          // Optionally add a cross at the center to mark the medial axis point
          bool crossSuccess1 = true;
          bool crossSuccess2 = true;

          if (params.crossSize > 0.0) {
            crossSuccess1 = sketch->addConstructionLine(x_world_mm - params.crossSize, y_world_mm,
                                                        x_world_mm + params.crossSize, y_world_mm);
            crossSuccess2 = sketch->addConstructionLine(x_world_mm, y_world_mm - params.crossSize, x_world_mm,
                                                        y_world_mm + params.crossSize);
          }

          if (!circleSuccess || !crossSuccess1 || !crossSuccess2) {
            // TODO(developer): Add implementation
          }
          totalCirclesDrawn++;
        }
        if (totalCirclesDrawn >= MAX_CONSTRUCTION_CIRCLES)
          break;
      }
    }

    // Enhanced UI Phase 5.3: Add polygonized shape outline
    if (params.showPolygonizedShape && !polygon.empty()) {
      // Draw construction lines connecting the polygon vertices
      for (size_t i = 0; i < polygon.size(); ++i) {
        const auto& p1 = polygon[i];
        const auto& p2 = polygon[(i + 1) % polygon.size()];  // Wrap around to close polygon

        // Polygon vertices are in world coordinates (cm) from
        // extractProfileGeometry Convert from cm to mm (FusionSketch methods
        // expect mm)
        double x1_world_mm = Utils::fusionLengthToMm(p1.x);
        double y1_world_mm = Utils::fusionLengthToMm(p1.y);
        double x2_world_mm = Utils::fusionLengthToMm(p2.x);
        double y2_world_mm = Utils::fusionLengthToMm(p2.y);

        // Add construction line directly in world coordinates
        bool success = sketch->addConstructionLine(x1_world_mm, y1_world_mm, x2_world_mm, y2_world_mm);
        if (!success) {
          // TODO(developer): Add implementation
        }
      }
    }
  } catch (const std::exception& e) {
  } catch (...) {
  }
}

}  // namespace Core
}  // namespace ChipCarving

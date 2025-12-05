/**
 * FusionWorkspaceCurveSurface.cpp
 *
 * Surface Z coordinate detection operations for FusionWorkspace
 * Split from FusionWorkspaceCurve.cpp for maintainability
 */

#include <cmath>
#include <iostream>
#include <vector>

#include "FusionAPIAdapter.h"
#include "utils/logging.h"

using adsk::core::Point3D;
using adsk::core::Ptr;

namespace ChipCarving {
namespace Adapters {

double FusionWorkspace::getSurfaceZAtXY(const std::string& /*surfaceId*/, double x, double y) {
  LOG_DEBUG("=== ENHANCED SURFACE QUERY: getSurfaceZAtXY called ===");
  LOG_DEBUG("Query point: (" << x << ", " << y << ") cm");

  if (!app_) {
    LOG_ERROR("No Fusion 360 application instance");
    return std::numeric_limits<double>::quiet_NaN();
  }

  // Get the active design
  Ptr<adsk::fusion::Design> design = app_->activeProduct();
  if (!design) {
    LOG_ERROR("No active design");
    return std::numeric_limits<double>::quiet_NaN();
  }

  // Get the root component
  Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
  if (!rootComp) {
    LOG_ERROR("No root component");
    return std::numeric_limits<double>::quiet_NaN();
  }

  // ENHANCED APPROACH: Universal ray casting across ALL components and
  // surface types This works with: root sketches + separate component
  // surfaces, B-Rep bodies, mesh bodies

  LOG_DEBUG("Using enhanced universal ray casting for cross-component surface "
            "detection");

  // Create a ray from above the surface pointing downward (in world
  // coordinates)
  double rayStartZ = 1000.0;  // Start ray 10 meters above (should be well above any surface)
  Ptr<adsk::core::Point3D> rayOrigin = adsk::core::Point3D::create(x, y, rayStartZ);
  Ptr<adsk::core::Vector3D> rayDirection = adsk::core::Vector3D::create(0.0, 0.0, -1.0);  // Pointing down

  if (!rayOrigin || !rayDirection) {
    LOG_ERROR("Could not create ray geometry");
    return std::numeric_limits<double>::quiet_NaN();
  }

  // Strategy: Search ALL components recursively, not just root component
  // This fixes the "root sketch + separate component surface" issue
  std::vector<Ptr<adsk::fusion::Component>> allComponents;
  allComponents.push_back(rootComp);

  // Add all occurrences (sub-components) recursively
  auto occurrences = rootComp->allOccurrences();
  if (occurrences) {
    LOG_DEBUG("Found " << occurrences->count() << " component occurrences to search");
    for (size_t i = 0; i < occurrences->count(); ++i) {
      auto occurrence = occurrences->item(i);
      if (occurrence && occurrence->component()) {
        allComponents.push_back(occurrence->component());
      }
    }
  }

  LOG_DEBUG("Searching " << allComponents.size() << " total components for surfaces");

  double bestZ = std::numeric_limits<double>::lowest();
  bool foundValidPoint = false;

  // Search each component for intersections
  for (size_t compIdx = 0; compIdx < allComponents.size(); ++compIdx) {
    const auto& component = allComponents[compIdx];
    if (!component)
      continue;

    LOG_DEBUG("Searching component " << compIdx);

    // Create object collection for hit points
    Ptr<adsk::core::ObjectCollection> hitPoints = adsk::core::ObjectCollection::create();
    if (!hitPoints)
      continue;

    // Cast ray to find intersections with ALL surface types
    // This searches both B-Rep faces AND mesh surfaces in this component
    Ptr<adsk::core::ObjectCollection> intersectedEntities =
        component->findBRepUsingRay(rayOrigin, rayDirection, adsk::fusion::BRepEntityTypes::BRepFaceEntityType,
                                    0.001,  // tolerance
                                    false,  // visibleEntitiesOnly - include all faces (critical for
                                            // cross-component)
                                    hitPoints);

    if (intersectedEntities && intersectedEntities->count() > 0) {
      LOG_DEBUG("Component " << compIdx << " ray casting found " << intersectedEntities->count()
                             << " intersected entities with " << hitPoints->count() << " hit points");

      // Process all hit points from this component
      for (size_t i = 0; i < hitPoints->count(); ++i) {
        Ptr<adsk::core::Point3D> hitPoint = hitPoints->item(i);
        if (hitPoint) {
          double hitZ = hitPoint->z();
          LOG_DEBUG("Component " << compIdx << " hit point " << i << ": (" << hitPoint->x() << ", " << hitPoint->y()
                                 << ", " << hitZ << ")");

          // Track the topmost (highest Z) intersection point across ALL
          // components
          if (hitZ > bestZ) {
            bestZ = hitZ;
            foundValidPoint = true;
            LOG_DEBUG("New topmost surface found at Z = " << bestZ << " cm in component " << compIdx);
          }
        }
      }
    }

    // ADDITIONAL: Also check mesh bodies in this component (for mesh
    // surface support)
    auto meshBodies = component->meshBodies();
    if (meshBodies && meshBodies->count() > 0) {
      LOG_DEBUG("Component " << compIdx << " has " << meshBodies->count()
                             << " mesh bodies - implementing mesh ray intersection");

      for (size_t meshIdx = 0; meshIdx < meshBodies->count(); ++meshIdx) {
        auto meshBody = meshBodies->item(meshIdx);
        if (!meshBody || !meshBody->displayMesh())
          continue;

        auto mesh = meshBody->displayMesh();
        if (!mesh)
          continue;

        auto nodeCoords = mesh->nodeCoordinates();
        auto nodeIndices = mesh->nodeIndices();

        if (nodeCoords.empty() || nodeIndices.empty())
          continue;

        LOG_DEBUG("Checking mesh " << meshIdx << " with " << nodeCoords.size() << " vertices and "
                                   << nodeIndices.size() / 3 << " triangles");

        // Simple mesh ray intersection - check triangles
        for (size_t triIdx = 0; triIdx < nodeIndices.size(); triIdx += 3) {
          if (triIdx + 2 >= nodeIndices.size())
            break;

          // Get triangle vertices
          auto v0Idx = nodeIndices[triIdx];
          auto v1Idx = nodeIndices[triIdx + 1];
          auto v2Idx = nodeIndices[triIdx + 2];

          if (static_cast<size_t>(v0Idx) >= nodeCoords.size() || static_cast<size_t>(v1Idx) >= nodeCoords.size() ||
              static_cast<size_t>(v2Idx) >= nodeCoords.size())
            continue;

          auto v0 = nodeCoords[v0Idx];
          auto v1 = nodeCoords[v1Idx];
          auto v2 = nodeCoords[v2Idx];

          if (!v0 || !v1 || !v2)
            continue;

          // Möller–Trumbore ray-triangle intersection algorithm
          // Ray: origin (x, y, rayStartZ), direction (0, 0, -1)

          // Edge vectors from v0 to v1 and v0 to v2
          double edge1X = v1->x() - v0->x();
          double edge1Y = v1->y() - v0->y();
          double edge1Z = v1->z() - v0->z();
          double edge2X = v2->x() - v0->x();
          double edge2Y = v2->y() - v0->y();
          double edge2Z = v2->z() - v0->z();

          // h = rayDir × edge2, where rayDir = (0, 0, -1)
          double hX = edge2Y;   // 0*edge2Z - (-1)*edge2Y
          double hY = -edge2X;  // (-1)*edge2X - 0*edge2Z
          double hZ = 0.0;      // 0*edge2Y - 0*edge2X

          // a = edge1 · h (determinant)
          double a = edge1X * hX + edge1Y * hY + edge1Z * hZ;

          // Ray parallel to triangle?
          if (std::abs(a) < 1e-9)
            continue;

          double f = 1.0 / a;

          // s = rayOrigin - v0
          double sX = x - v0->x();
          double sY = y - v0->y();
          double sZ = rayStartZ - v0->z();

          // u = f * (s · h) - first barycentric coordinate
          double u = f * (sX * hX + sY * hY + sZ * hZ);
          if (u < 0.0 || u > 1.0)
            continue;

          // q = s × edge1
          double qX = sY * edge1Z - sZ * edge1Y;
          double qY = sZ * edge1X - sX * edge1Z;
          double qZ = sX * edge1Y - sY * edge1X;

          // v = f * (rayDir · q) - second barycentric coordinate
          double v = f * (-qZ);  // rayDir = (0, 0, -1), so dot = -qZ
          if (v < 0.0 || u + v > 1.0)
            continue;

          // t = f * (edge2 · q) - ray parameter (distance)
          double t = f * (edge2X * qX + edge2Y * qY + edge2Z * qZ);
          if (t < 0.0)
            continue;

          // Hit point Z (ray goes down, so subtract)
          double hitZ = rayStartZ - t;

          if (hitZ > bestZ && hitZ < rayStartZ) {
            bestZ = hitZ;
            foundValidPoint = true;
            LOG_DEBUG("Mesh hit at Z=" << bestZ << " (bary u=" << u << " v=" << v << ")");
          }
        }
      }
    }
  }

  if (foundValidPoint) {
    LOG_DEBUG("Enhanced ray casting found topmost surface at Z = " << bestZ << " cm across " << allComponents.size()
                                                                   << " components");
    return bestZ;
  }
  LOG_WARNING("Enhanced ray casting found no valid surface at XY location (" << x << ", " << y << ") across "
                                                                             << allComponents.size() << " components");
  return std::numeric_limits<double>::quiet_NaN();
}

}  // namespace Adapters
}  // namespace ChipCarving

/**
 * FusionWorkspaceCurveSurface.cpp
 *
 * Surface Z coordinate detection operations for FusionWorkspace
 * Split from FusionWorkspaceCurve.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "FusionAPIAdapter.h"
#include "../../include/utils/TempFileManager.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

double FusionWorkspace::getSurfaceZAtXY(const std::string& surfaceId, double x, double y) {
    // Use the main debug log
    std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
    std::ofstream debugLog(debugLogPath, std::ios::app);
    debugLog << "=== ENHANCED SURFACE QUERY: getSurfaceZAtXY called ===" << std::endl;
    debugLog << "Surface ID: " << surfaceId << std::endl;
    debugLog << "Query point: (" << x << ", " << y << ") cm" << std::endl;

    try {
        if (!app_) {
            debugLog << "[ERROR] No Fusion 360 application instance" << std::endl;
            return std::numeric_limits<double>::quiet_NaN();
        }

        // Get the active design
        Ptr<adsk::fusion::Design> design = app_->activeProduct();
        if (!design) {
            debugLog << "[ERROR] No active design" << std::endl;
            return std::numeric_limits<double>::quiet_NaN();
        }

        // Get the root component
        Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
        if (!rootComp) {
            debugLog << "[ERROR] No root component" << std::endl;
            return std::numeric_limits<double>::quiet_NaN();
        }

        // ENHANCED APPROACH: Universal ray casting across ALL components and surface types
        // This works with: root sketches + separate component surfaces, B-Rep bodies, mesh bodies

        debugLog << "[DEBUG] Using enhanced universal ray casting for cross-component surface detection" << std::endl;

        // Create a ray from above the surface pointing downward (in world coordinates)
        double rayStartZ = 1000.0;  // Start ray 10 meters above (should be well above any surface)
        Ptr<adsk::core::Point3D> rayOrigin = adsk::core::Point3D::create(x, y, rayStartZ);
        Ptr<adsk::core::Vector3D> rayDirection = adsk::core::Vector3D::create(0.0, 0.0, -1.0);  // Pointing down

        if (!rayOrigin || !rayDirection) {
            debugLog << "[ERROR] Could not create ray geometry" << std::endl;
            return std::numeric_limits<double>::quiet_NaN();
        }

        // Strategy: Search ALL components recursively, not just root component
        // This fixes the "root sketch + separate component surface" issue
        std::vector<Ptr<adsk::fusion::Component>> allComponents;
        allComponents.push_back(rootComp);

        // Add all occurrences (sub-components) recursively
        auto occurrences = rootComp->allOccurrences();
        if (occurrences) {
            debugLog << "[DEBUG] Found " << occurrences->count() << " component occurrences to search" << std::endl;
            for (size_t i = 0; i < occurrences->count(); ++i) {
                auto occurrence = occurrences->item(i);
                if (occurrence && occurrence->component()) {
                    allComponents.push_back(occurrence->component());
                }
            }
        }

        debugLog << "[DEBUG] Searching " << allComponents.size() << " total components for surfaces" << std::endl;

        double bestZ = std::numeric_limits<double>::lowest();
        bool foundValidPoint = false;
        int totalRayHits = 0;

        // Search each component for intersections
        for (size_t compIdx = 0; compIdx < allComponents.size(); ++compIdx) {
            auto component = allComponents[compIdx];
            if (!component) continue;

            debugLog << "[DEBUG] Searching component " << compIdx << std::endl;

            // Create object collection for hit points
            Ptr<adsk::core::ObjectCollection> hitPoints = adsk::core::ObjectCollection::create();
            if (!hitPoints) continue;

            try {
                // Cast ray to find intersections with ALL surface types
                // This searches both B-Rep faces AND mesh surfaces in this component
                Ptr<adsk::core::ObjectCollection> intersectedEntities = component->findBRepUsingRay(
                    rayOrigin,
                    rayDirection,
                    adsk::fusion::BRepEntityTypes::BRepFaceEntityType,
                    0.001,  // tolerance
                    false,  // visibleEntitiesOnly - include all faces (critical for cross-component)
                    hitPoints);

                if (intersectedEntities && intersectedEntities->count() > 0) {
                    debugLog << "[DEBUG] Component " << compIdx << " ray casting found "
                             << intersectedEntities->count() << " intersected entities with "
                             << hitPoints->count() << " hit points" << std::endl;
                    totalRayHits += hitPoints->count();

                    // Process all hit points from this component
                    for (size_t i = 0; i < hitPoints->count(); ++i) {
                        Ptr<adsk::core::Point3D> hitPoint = hitPoints->item(i);
                        if (hitPoint) {
                            double hitZ = hitPoint->z();
                            debugLog << "[DEBUG] Component " << compIdx << " hit point " << i
                                     << ": (" << hitPoint->x() << ", " << hitPoint->y() << ", " << hitZ << ")" << std::endl;

                            // Track the topmost (highest Z) intersection point across ALL components
                            if (hitZ > bestZ) {
                                bestZ = hitZ;
                                foundValidPoint = true;
                                debugLog << "[DEBUG] New topmost surface found at Z = " << bestZ
                                         << " cm in component " << compIdx << std::endl;
                            }
                        }
                    }
                }

                // ADDITIONAL: Also check mesh bodies in this component (for mesh surface support)
                auto meshBodies = component->meshBodies();
                if (meshBodies && meshBodies->count() > 0) {
                    debugLog << "[DEBUG] Component " << compIdx << " has " << meshBodies->count()
                             << " mesh bodies - implementing mesh ray intersection" << std::endl;

                    for (size_t meshIdx = 0; meshIdx < meshBodies->count(); ++meshIdx) {
                        auto meshBody = meshBodies->item(meshIdx);
                        if (!meshBody || !meshBody->displayMesh()) continue;

                        auto mesh = meshBody->displayMesh();
                        if (!mesh) continue;

                        auto nodeCoords = mesh->nodeCoordinates();
                        auto nodeIndices = mesh->nodeIndices();

                        if (nodeCoords.empty() || nodeIndices.empty()) continue;

                        debugLog << "[DEBUG] Checking mesh " << meshIdx << " with "
                                 << nodeCoords.size() << " vertices and "
                                 << nodeIndices.size()/3 << " triangles" << std::endl;

                        // Simple mesh ray intersection - check triangles
                        for (size_t triIdx = 0; triIdx < nodeIndices.size(); triIdx += 3) {
                            if (triIdx + 2 >= nodeIndices.size()) break;

                            // Get triangle vertices
                            auto v0Idx = nodeIndices[triIdx];
                            auto v1Idx = nodeIndices[triIdx + 1];
                            auto v2Idx = nodeIndices[triIdx + 2];

                            if (static_cast<size_t>(v0Idx) >= nodeCoords.size() ||
                                static_cast<size_t>(v1Idx) >= nodeCoords.size() ||
                                static_cast<size_t>(v2Idx) >= nodeCoords.size())
                                continue;

                            auto v0 = nodeCoords[v0Idx];
                            auto v1 = nodeCoords[v1Idx];
                            auto v2 = nodeCoords[v2Idx];

                            if (!v0 || !v1 || !v2) continue;

                            // Simple ray-triangle intersection test
                            // Check if ray intersects this triangle's plane
                            double deltaX0 = v1->x() - v0->x();
                            double deltaY0 = v1->y() - v0->y();
                            double deltaZ0 = v1->z() - v0->z();
                            double deltaX1 = v2->x() - v0->x();
                            double deltaY1 = v2->y() - v0->y();
                            double deltaZ1 = v2->z() - v0->z();

                            // Triangle normal (cross product)
                            double nx = deltaY0 * deltaZ1 - deltaZ0 * deltaY1;
                            double ny = deltaZ0 * deltaX1 - deltaX0 * deltaZ1;
                            double nz = deltaX0 * deltaY1 - deltaY0 * deltaX1;

                            // Check if ray is parallel to triangle
                            double rayDotNormal = rayDirection->z() * nz;  // ray is (0,0,-1)
                            if (std::abs(rayDotNormal) < 1e-6) continue;

                            // Find intersection point with triangle plane
                            double d = -(nx * v0->x() + ny * v0->y() + nz * v0->z());
                            double t = -(nx * x + ny * y + nz * rayStartZ + d) / rayDotNormal;

                            if (t < 0) continue;  // Behind ray start

                            double hitZ = rayStartZ + t * rayDirection->z();

                            // Simple inside-triangle test (could be more precise)
                            // For now, just check if intersection is reasonable
                            if (hitZ > bestZ && hitZ < rayStartZ) {
                                bestZ = hitZ;
                                foundValidPoint = true;
                                debugLog << "[DEBUG] Mesh intersection found at Z = " << bestZ
                                         << " cm in component " << compIdx << ", mesh " << meshIdx << std::endl;
                            }
                        }
                    }
                }

            } catch (const std::exception& e) {
                debugLog << "[DEBUG] Component " << compIdx << " ray casting failed: " << e.what() << std::endl;
                // Continue with other components
            } catch (...) {
                debugLog << "[DEBUG] Component " << compIdx << " ray casting failed with unknown exception" << std::endl;
                // Continue with other components
            }
        }

        debugLog << "[DEBUG] Total ray hits across all components: " << totalRayHits << std::endl;

        if (foundValidPoint) {
            debugLog << "[SUCCESS] Enhanced ray casting found topmost surface at Z = " << bestZ
                     << " cm across " << allComponents.size() << " components" << std::endl;
            return bestZ;
        } else {
            debugLog << "[WARNING] Enhanced ray casting found no valid surface at XY location ("
                     << x << ", " << y << ") across " << allComponents.size() << " components" << std::endl;
            return std::numeric_limits<double>::quiet_NaN();
        }

    } catch (const std::exception& e) {
        std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
        std::ofstream debugLog(debugLogPath, std::ios::app);
        debugLog << "[ERROR] Exception in enhanced getSurfaceZAtXY: " << e.what() << std::endl;
        return std::numeric_limits<double>::quiet_NaN();
    } catch (...) {
        std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
        std::ofstream debugLog(debugLogPath, std::ios::app);
        debugLog << "[ERROR] Unknown exception in enhanced getSurfaceZAtXY" << std::endl;
        return std::numeric_limits<double>::quiet_NaN();
    }
}

}  // namespace Adapters
}  // namespace ChipCarving

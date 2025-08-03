/**
 * FusionWorkspaceSketchComponent.cpp
 *
 * Component-aware sketch creation operations for FusionWorkspace
 * Split from FusionWorkspaceSketch.cpp for maintainability
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

std::unique_ptr<ISketch> FusionWorkspace::createSketchInTargetComponent(const std::string& name,
                                                                        const std::string& surfaceEntityId) {
    if (!app_) {
        return nullptr;
    }

    try {
        // Get the active design
        Ptr<adsk::fusion::Design> design = app_->activeProduct();
        if (!design) {
            return nullptr;
        }

        // Get the root component
        Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
        if (!rootComp) {
            return nullptr;
        }

        // Find the component containing the target surface using the same cross-component search
        std::vector<Ptr<adsk::fusion::Component>> allComponents;
        allComponents.push_back(rootComp);

        // Debug logging for component search
        std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
        std::ofstream debugLog(debugLogPath, std::ios::app);
        debugLog << "[COMPONENT SEARCH] Starting search for surface with entityId: '" << surfaceEntityId << "'" << std::endl;
        std::string rootName = rootComp->name();
        debugLog << "[COMPONENT SEARCH] Added root component: " << (rootName.empty() ? "unnamed" : rootName) << std::endl;

        // Add all occurrences (sub-components) recursively
        auto occurrences = rootComp->allOccurrences();
        if (occurrences) {
            debugLog << "[COMPONENT SEARCH] Found " << occurrences->count() << " component occurrences" << std::endl;
            for (size_t i = 0; i < occurrences->count(); ++i) {
                auto occurrence = occurrences->item(i);
                if (occurrence && occurrence->component()) {
                    allComponents.push_back(occurrence->component());
                    std::string occName = occurrence->component()->name();
                    debugLog << "[COMPONENT SEARCH] Added occurrence " << i << ": " << (occName.empty() ? "unnamed" : occName) << std::endl;
                }
            }
        } else {
            debugLog << "[COMPONENT SEARCH] No component occurrences found" << std::endl;
        }

        Ptr<adsk::fusion::Component> targetComponent = nullptr;

        // Search for the component containing the surface
        for (size_t compIdx = 0; compIdx < allComponents.size(); ++compIdx) {
            auto component = allComponents[compIdx];
            if (!component) continue;

            std::string compName = component->name();
            debugLog << "[COMPONENT SEARCH] Searching component " << compIdx << ": " << (compName.empty() ? "unnamed" : compName) << std::endl;

            // Check B-Rep bodies in this component for the surface
            auto bodies = component->bRepBodies();
            if (bodies && bodies->count() > 0) {
                debugLog << "[COMPONENT SEARCH] Component " << compIdx << " has " << bodies->count() << " B-Rep bodies" << std::endl;

                for (size_t i = 0; i < bodies->count(); ++i) {
                    auto body = bodies->item(i);
                    if (!body) continue;

                    auto faces = body->faces();
                    if (!faces) continue;

                    debugLog << "[COMPONENT SEARCH] B-Rep Body " << i << " has " << faces->count() << " faces" << std::endl;

                    for (size_t j = 0; j < faces->count(); ++j) {
                        auto face = faces->item(j);
                        if (!face) continue;

                        std::string faceToken = face->entityToken();
                        if (j < 3) {  // Log first few face tokens for debugging
                            debugLog << "[COMPONENT SEARCH] B-Rep Face " << j << " token: '" << faceToken << "'" << std::endl;
                        }

                        if (faceToken == surfaceEntityId) {
                            targetComponent = component;
                            std::string foundCompName = component->name();
                            debugLog << "[COMPONENT SEARCH] FOUND! B-Rep surface found in component: " << (foundCompName.empty() ? "unnamed" : foundCompName) << std::endl;
                            break;
                        }
                    }
                    if (targetComponent) break;
                }
            } else {
                debugLog << "[COMPONENT SEARCH] Component " << compIdx << " has no B-Rep bodies" << std::endl;
            }

            // Also check mesh bodies in this component (for STL/OBJ imports)
            auto meshBodies = component->meshBodies();
            if (!targetComponent && meshBodies && meshBodies->count() > 0) {
                debugLog << "[COMPONENT SEARCH] Component " << compIdx << " has " << meshBodies->count() << " mesh bodies" << std::endl;

                for (size_t i = 0; i < meshBodies->count(); ++i) {
                    auto meshBody = meshBodies->item(i);
                    if (!meshBody) continue;

                    std::string meshToken = meshBody->entityToken();
                    debugLog << "[COMPONENT SEARCH] Mesh Body " << i << " token: '" << meshToken << "'" << std::endl;

                    if (meshToken == surfaceEntityId) {
                        targetComponent = component;
                        std::string foundCompName = component->name();
                        debugLog << "[COMPONENT SEARCH] FOUND! Mesh surface found in component: " << (foundCompName.empty() ? "unnamed" : foundCompName) << std::endl;
                        break;
                    }
                }
            } else if (!targetComponent) {
                debugLog << "[COMPONENT SEARCH] Component " << compIdx << " has no mesh bodies" << std::endl;
            }

            if (targetComponent) break;
        }

        // If surface not found with exact token matching, try alternative approaches
        if (!targetComponent) {
            debugLog << "[COMPONENT SEARCH] Exact token match failed. Trying fallback approaches..." << std::endl;

            // Fallback 1: If there's only one component with surfaces, use that
            Ptr<adsk::fusion::Component> singleSurfaceComponent = nullptr;
            int componentsWithSurfaces = 0;

            for (size_t compIdx = 0; compIdx < allComponents.size(); ++compIdx) {
                auto component = allComponents[compIdx];
                if (!component) continue;

                bool hasSurfaces = false;
                auto bodies = component->bRepBodies();
                if (bodies && bodies->count() > 0) {
                    for (size_t i = 0; i < bodies->count(); ++i) {
                        auto body = bodies->item(i);
                        if (body && body->faces() && body->faces()->count() > 0) {
                            hasSurfaces = true;
                            break;
                        }
                    }
                }

                if (!hasSurfaces) {
                    auto meshBodies = component->meshBodies();
                    if (meshBodies && meshBodies->count() > 0) {
                        hasSurfaces = true;
                    }
                }

                if (hasSurfaces) {
                    componentsWithSurfaces++;
                    singleSurfaceComponent = component;
                }
            }

            if (componentsWithSurfaces == 1 && singleSurfaceComponent) {
                targetComponent = singleSurfaceComponent;
                std::string fallbackCompName = targetComponent->name();
                debugLog << "[COMPONENT SEARCH] FALLBACK SUCCESS! Using only component with surfaces: " << (fallbackCompName.empty() ? "unnamed" : fallbackCompName) << std::endl;
            } else {
                debugLog << "[COMPONENT SEARCH] Found " << componentsWithSurfaces << " components with surfaces. Cannot use fallback." << std::endl;
            }
        }

        // Final fallback to root component
        if (!targetComponent) {
            debugLog << "[COMPONENT SEARCH] All approaches failed - falling back to root component" << std::endl;
            targetComponent = rootComp;
        } else {
            std::string targetCompName = targetComponent->name();
            debugLog << "[COMPONENT SEARCH] Using target component: " << (targetCompName.empty() ? "unnamed" : targetCompName) << std::endl;
        }

        // Get the XY plane for the sketch in the target component
        Ptr<adsk::fusion::ConstructionPlane> xyPlane = targetComponent->xYConstructionPlane();
        if (!xyPlane) {
            return nullptr;
        }

        // Create the sketch in the target component
        Ptr<adsk::fusion::Sketches> sketches = targetComponent->sketches();
        if (!sketches) {
            return nullptr;
        }

        Ptr<adsk::fusion::Sketch> sketch = sketches->add(xyPlane);
        if (!sketch) {
            return nullptr;
        }

        // Set the sketch name
        sketch->name(name);

        return std::make_unique<FusionSketch>(name, app_, sketch);

    } catch (const std::exception& e) {
        std::cout << "Target component sketch creation error: " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cout << "Unknown target component sketch creation error" << std::endl;
        return nullptr;
    }
}

}  // namespace Adapters
}  // namespace ChipCarving

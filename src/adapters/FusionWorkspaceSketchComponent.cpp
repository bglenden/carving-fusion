/**
 * FusionWorkspaceSketchComponent.cpp
 *
 * Component-aware sketch creation operations for FusionWorkspace
 * Split from FusionWorkspaceSketch.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#include "../../include/utils/logging.h"
#include "FusionAPIAdapter.h"

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

    // Find the component containing the target surface using the same
    // cross-component search
    std::vector<Ptr<adsk::fusion::Component>> allComponents;
    allComponents.push_back(rootComp);

    // Debug logging for component search
    LOG_DEBUG("Starting search for surface with entityId: '" << surfaceEntityId << "'");
    std::string rootName = rootComp->name();
    LOG_DEBUG("Added root component: " << (rootName.empty() ? "unnamed" : rootName));

    // Add all occurrences (sub-components) recursively
    auto occurrences = rootComp->allOccurrences();
    if (occurrences) {
      LOG_DEBUG("Found " << occurrences->count() << " component occurrences");
      for (size_t i = 0; i < occurrences->count(); ++i) {
        auto occurrence = occurrences->item(i);
        if (occurrence && occurrence->component()) {
          allComponents.push_back(occurrence->component());
          std::string occName = occurrence->component()->name();
          LOG_DEBUG("Added occurrence " << i << ": " << (occName.empty() ? "unnamed" : occName));
        }
      }
    } else {
      LOG_DEBUG("No component occurrences found");
    }

    Ptr<adsk::fusion::Component> targetComponent = nullptr;

    // Search for the component containing the surface
    for (size_t compIdx = 0; compIdx < allComponents.size(); ++compIdx) {
      auto component = allComponents[compIdx];
      if (!component)
        continue;

      std::string compName = component->name();
      LOG_DEBUG("Searching component " << compIdx << ": " << (compName.empty() ? "unnamed" : compName));

      // Check B-Rep bodies in this component for the surface
      auto bodies = component->bRepBodies();
      if (bodies && bodies->count() > 0) {
        LOG_DEBUG("Component " << compIdx << " has " << bodies->count() << " B-Rep bodies");

        for (size_t i = 0; i < bodies->count(); ++i) {
          auto body = bodies->item(i);
          if (!body)
            continue;

          auto faces = body->faces();
          if (!faces)
            continue;

          LOG_DEBUG("B-Rep Body " << i << " has " << faces->count() << " faces");

          for (size_t j = 0; j < faces->count(); ++j) {
            auto face = faces->item(j);
            if (!face)
              continue;

            std::string faceToken = face->entityToken();
            if (j < 3) {  // Log first few face tokens for debugging
              LOG_DEBUG("B-Rep Face " << j << " token: '" << faceToken << "'");
            }

            if (faceToken == surfaceEntityId) {
              targetComponent = component;
              std::string foundCompName = component->name();
              LOG_DEBUG(
                  "FOUND! B-Rep surface found in component: " << (foundCompName.empty() ? "unnamed" : foundCompName));
              break;
            }
          }
          if (targetComponent)
            break;
        }
      } else {
        LOG_DEBUG("Component " << compIdx << " has no B-Rep bodies");
      }

      // Also check mesh bodies in this component (for STL/OBJ imports)
      auto meshBodies = component->meshBodies();
      if (!targetComponent && meshBodies && meshBodies->count() > 0) {
        LOG_DEBUG("Component " << compIdx << " has " << meshBodies->count() << " mesh bodies");

        for (size_t i = 0; i < meshBodies->count(); ++i) {
          auto meshBody = meshBodies->item(i);
          if (!meshBody)
            continue;

          std::string meshToken = meshBody->entityToken();
          LOG_DEBUG("Mesh Body " << i << " token: '" << meshToken << "'");

          if (meshToken == surfaceEntityId) {
            targetComponent = component;
            std::string foundCompName = component->name();
            LOG_DEBUG(
                "FOUND! Mesh surface found in component: " << (foundCompName.empty() ? "unnamed" : foundCompName));
            break;
          }
        }
      } else if (!targetComponent) {
        LOG_DEBUG("Component " << compIdx << " has no mesh bodies");
      }

      if (targetComponent)
        break;
    }

    // If surface not found with exact token matching, try alternative
    // approaches
    if (!targetComponent) {
      LOG_DEBUG("Exact token match failed. Trying fallback approaches...");

      // Fallback 1: If there's only one component with surfaces, use that
      Ptr<adsk::fusion::Component> singleSurfaceComponent = nullptr;
      int componentsWithSurfaces = 0;

      for (size_t compIdx = 0; compIdx < allComponents.size(); ++compIdx) {
        auto component = allComponents[compIdx];
        if (!component)
          continue;

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
        LOG_DEBUG("FALLBACK SUCCESS! Using only component with surfaces: "
                  << (fallbackCompName.empty() ? "unnamed" : fallbackCompName));
      } else {
        LOG_DEBUG("Found " << componentsWithSurfaces << " components with surfaces. Cannot use fallback.");
      }
    }

    // Final fallback to root component
    if (!targetComponent) {
      LOG_DEBUG("All approaches failed - falling back to root component");
      targetComponent = rootComp;
    } else {
      std::string targetCompName = targetComponent->name();
      LOG_DEBUG("Using target component: " << (targetCompName.empty() ? "unnamed" : targetCompName));
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

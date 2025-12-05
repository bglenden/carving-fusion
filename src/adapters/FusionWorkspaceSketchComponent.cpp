/**
 * FusionWorkspaceSketchComponent.cpp
 *
 * Component-aware sketch creation operations for FusionWorkspace
 *
 * REFACTORED: Now uses Design.findEntityByToken() for direct entity lookup
 * instead of manually iterating through all components/bodies/faces.
 *
 * Previous implementation preserved in git history - see commit message
 * for details on reverting if issues arise.
 */

#include "FusionAPIAdapter.h"
#include "utils/logging.h"

using adsk::core::Base;
using adsk::core::Ptr;

namespace ChipCarving {
namespace Adapters {

std::unique_ptr<ISketch> FusionWorkspace::createSketchInTargetComponent(const std::string& name,
                                                                        const std::string& surfaceEntityId) {
  if (!app_) {
    return nullptr;
  }

  // Get the active design
  Ptr<adsk::fusion::Design> design = app_->activeProduct();
  if (!design) {
    LOG_ERROR("createSketchInTargetComponent: No active design");
    return nullptr;
  }

  // Get the root component (used as fallback)
  Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
  if (!rootComp) {
    LOG_ERROR("createSketchInTargetComponent: No root component");
    return nullptr;
  }

  Ptr<adsk::fusion::Component> targetComponent = nullptr;

  // ========================================================================
  // DIRECT ENTITY LOOKUP using Design.findEntityByToken()
  // This replaces ~100 lines of manual iteration through components/bodies/faces
  // ========================================================================

  if (!surfaceEntityId.empty()) {
    LOG_DEBUG("Looking up surface entity directly: " << surfaceEntityId);

    // Use the official Fusion API for O(1) entity lookup
    std::vector<Ptr<Base>> entities = findEntitiesByToken(surfaceEntityId);

    if (!entities.empty()) {
      // Get the parent component from the found entity
      targetComponent = getComponentFromEntity(entities[0]);

      if (targetComponent) {
        LOG_DEBUG("FOUND via direct lookup! Component: " << targetComponent->name());
      } else {
        LOG_WARNING("Entity found but could not determine parent component");
      }
    } else {
      LOG_WARNING("Direct entity lookup found no match for token: " << surfaceEntityId);
    }
  }

  // Fallback to root component if direct lookup fails
  if (!targetComponent) {
    LOG_DEBUG("Using root component as fallback");
    targetComponent = rootComp;
  }

  // Get the XY plane for the sketch in the target component
  Ptr<adsk::fusion::ConstructionPlane> xyPlane = targetComponent->xYConstructionPlane();
  if (!xyPlane) {
    LOG_ERROR("Could not get XY plane from target component");
    return nullptr;
  }

  // Create the sketch in the target component
  Ptr<adsk::fusion::Sketches> sketches = targetComponent->sketches();
  if (!sketches) {
    LOG_ERROR("Could not get sketches collection from target component");
    return nullptr;
  }

  Ptr<adsk::fusion::Sketch> sketch = sketches->add(xyPlane);
  if (!sketch) {
    logApiError("sketches->add(xyPlane)");
    return nullptr;
  }

  // Set the sketch name
  sketch->name(name);

  LOG_DEBUG("Created sketch '" << name << "' in component: " << targetComponent->name());
  return std::make_unique<FusionSketch>(name, app_, sketch);
}

}  // namespace Adapters
}  // namespace ChipCarving

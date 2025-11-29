/**
 * FusionWorkspaceSketchPlane.cpp
 *
 * Plane-based sketch creation and finding operations for FusionWorkspace
 *
 * REFACTORED: Now uses Design.findEntityByToken() for direct plane lookup
 * instead of manually iterating through construction planes and faces.
 *
 * Previous implementation preserved in git history - see commit message
 * for details on reverting if issues arise.
 */

#include <cmath>
#include <iostream>

#include "utils/logging.h"
#include "FusionAPIAdapter.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

std::unique_ptr<ISketch> FusionWorkspace::createSketchOnPlane(const std::string& name,
                                                              const std::string& planeEntityId) {
  if (!app_) {
    return nullptr;
  }

  try {
    // Get the active design
    Ptr<adsk::fusion::Design> design = app_->activeProduct();
    if (!design) {
      LOG_ERROR("createSketchOnPlane: No active design");
      return nullptr;
    }

    // Get the root component
    Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
    if (!rootComp) {
      LOG_ERROR("createSketchOnPlane: No root component");
      return nullptr;
    }

    // Get the sketches collection
    Ptr<adsk::fusion::Sketches> sketches = rootComp->sketches();
    if (!sketches) {
      LOG_ERROR("createSketchOnPlane: No sketches collection");
      return nullptr;
    }

    // ========================================================================
    // DIRECT ENTITY LOOKUP using Design.findEntityByToken()
    // This replaces manual iteration through construction planes and faces
    // ========================================================================

    Ptr<Base> planeEntity = nullptr;

    if (planeEntityId.empty()) {
      // No plane specified, use XY plane
      LOG_DEBUG("No plane entity ID provided, using XY plane");
      planeEntity = rootComp->xYConstructionPlane();
    } else {
      LOG_DEBUG("Looking up plane entity directly: " << planeEntityId);

      // Use the official Fusion API for O(1) entity lookup
      std::vector<Ptr<Base>> entities = findEntitiesByToken(planeEntityId);

      if (!entities.empty()) {
        planeEntity = entities[0];
        LOG_DEBUG("FOUND plane entity via direct lookup. Type: " << planeEntity->objectType());
      } else {
        LOG_WARNING("Direct plane lookup failed for token: " << planeEntityId);
      }
    }

    // Fall back to XY plane if entity not found
    if (!planeEntity) {
      LOG_DEBUG("Falling back to XY plane");
      planeEntity = rootComp->xYConstructionPlane();
    }

    // ========================================================================
    // PLANE VALIDATION: Check if plane is parallel to XY
    // This logic is preserved from the original implementation
    // ========================================================================

    bool isValidPlane = false;
    double planeZ = 0.0;

    // Check if it's a construction plane
    Ptr<adsk::fusion::ConstructionPlane> constructionPlane = planeEntity;
    if (constructionPlane) {
      Ptr<adsk::core::Plane> geometry = constructionPlane->geometry();
      if (geometry) {
        Ptr<Vector3D> normal = geometry->normal();
        if (normal) {
          // Check if normal is parallel to Z axis (0, 0, ±1)
          double tolerance = 0.001;
          if (std::abs(normal->x()) < tolerance && std::abs(normal->y()) < tolerance &&
              std::abs(std::abs(normal->z()) - 1.0) < tolerance) {
            isValidPlane = true;
            Ptr<Point3D> origin = geometry->origin();
            if (origin) {
              planeZ = origin->z();
              LOG_DEBUG("Construction plane Z position: " << planeZ);
            }
          }
        }
      }
    } else {
      // Check if it's a planar face
      Ptr<adsk::fusion::BRepFace> face = planeEntity;
      if (face) {
        Ptr<adsk::core::Surface> surface = face->geometry();
        if (surface) {
          // Check if it's a plane
          Ptr<adsk::core::Plane> plane = surface;
          if (plane) {
            Ptr<Vector3D> normal = plane->normal();
            if (normal) {
              // Check if normal is parallel to Z axis
              double tolerance = 0.001;
              if (std::abs(normal->x()) < tolerance && std::abs(normal->y()) < tolerance &&
                  std::abs(std::abs(normal->z()) - 1.0) < tolerance) {
                isValidPlane = true;
                Ptr<Point3D> origin = plane->origin();
                if (origin) {
                  planeZ = origin->z();
                  LOG_DEBUG("BRep face Z position: " << planeZ);
                }
              }
            }
          }
        }
      }
    }

    // Suppress unused variable warning
    (void)planeZ;

    if (!isValidPlane && !planeEntityId.empty()) {
      LOG_ERROR("Selected plane/surface is not parallel to XY plane. Using XY plane instead.");
      planeEntity = rootComp->xYConstructionPlane();
    }

    // Create the sketch on the plane
    Ptr<adsk::fusion::Sketch> sketch = sketches->add(planeEntity);
    if (!sketch) {
      logApiError("sketches->add(planeEntity)");
      return nullptr;
    }

    // Set the sketch name
    sketch->name(name);

    LOG_DEBUG("Created sketch '" << name << "' on plane");
    return std::make_unique<FusionSketch>(name, app_, sketch);
  } catch (const std::exception& e) {
    LOG_ERROR("Sketch creation on plane error: " << e.what());
    return nullptr;
  } catch (...) {
    LOG_ERROR("Unknown sketch creation on plane error");
    return nullptr;
  }
}

std::unique_ptr<ISketch> FusionWorkspace::findSketch(const std::string& name) {
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

    // Get all sketches
    Ptr<adsk::fusion::Sketches> sketches = rootComp->sketches();
    if (!sketches) {
      return nullptr;
    }

    // Search for sketch by name
    for (size_t i = 0; i < sketches->count(); ++i) {
      Ptr<adsk::fusion::Sketch> sketch = sketches->item(i);
      if (sketch && sketch->name() == name) {
        // Found existing sketch with matching name
        return std::make_unique<FusionSketch>(name, app_, sketch);
      }
    }

    // No sketch found with this name
    return nullptr;
  } catch (const std::exception& e) {
    LOG_ERROR("Find sketch error: " << e.what());
    return nullptr;
  } catch (...) {
    LOG_ERROR("Unknown find sketch error");
    return nullptr;
  }
}

}  // namespace Adapters
}  // namespace ChipCarving

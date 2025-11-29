/**
 * FusionWorkspaceEntityLookup.cpp
 *
 * Direct entity lookup using Design.findEntityByToken() API
 *
 * This replaces the previous approach of manually iterating through all
 * components, bodies, and faces to find entities by token comparison.
 *
 * WHY THIS CHANGE:
 * - Design.findEntityByToken() is the official API method for this purpose
 * - O(1) lookup vs O(components x bodies x faces) manual iteration
 * - Simpler, more maintainable code
 * - Handles edge cases that manual iteration might miss
 *
 * IF YOU EXPERIENCE ISSUES:
 * The previous manual iteration approach is preserved in git history.
 * See the commit that introduced this file for the old implementation.
 */

#include "utils/logging.h"
#include "FusionAPIAdapter.h"

using adsk::core::Base;
using adsk::core::Ptr;

namespace ChipCarving {
namespace Adapters {

std::vector<Ptr<Base>> FusionWorkspace::findEntitiesByToken(const std::string& entityToken) {
  std::vector<Ptr<Base>> result;

  if (entityToken.empty()) {
    LOG_DEBUG("findEntitiesByToken called with empty token");
    return result;
  }

  if (!app_) {
    LOG_ERROR("findEntitiesByToken: No Fusion 360 application instance");
    return result;
  }

  try {
    // Get the active design
    Ptr<adsk::fusion::Design> design = app_->activeProduct();
    if (!design) {
      LOG_ERROR("findEntitiesByToken: No active design");
      return result;
    }

    // Use the official Fusion API method for direct lookup
    // This is O(1) - Fusion maintains internal index of entity tokens
    result = design->findEntityByToken(entityToken);

    if (result.empty()) {
      LOG_DEBUG("findEntitiesByToken: No entity found for token: " << entityToken);
    } else {
      LOG_DEBUG("findEntitiesByToken: Found " << result.size() << " entity(ies) for token");
    }

    return result;
  } catch (const std::exception& e) {
    LOG_ERROR("findEntitiesByToken exception: " << e.what());
    return result;
  } catch (...) {
    LOG_ERROR("findEntitiesByToken: Unknown exception");
    return result;
  }
}

Ptr<adsk::fusion::Component> FusionWorkspace::getComponentFromEntity(Ptr<Base> entity) {
  if (!entity) {
    return nullptr;
  }

  try {
    std::string entityType = entity->objectType();
    LOG_DEBUG("getComponentFromEntity: Entity type is " << entityType);

    // Handle BRepFace -> body -> parentComponent
    if (Ptr<adsk::fusion::BRepFace> face = entity) {
      Ptr<adsk::fusion::BRepBody> body = face->body();
      if (body) {
        Ptr<adsk::fusion::Component> comp = body->parentComponent();
        if (comp) {
          LOG_DEBUG("getComponentFromEntity: Found component from BRepFace: " << comp->name());
          return comp;
        }
      }
    }

    // Handle BRepBody -> parentComponent
    if (Ptr<adsk::fusion::BRepBody> body = entity) {
      Ptr<adsk::fusion::Component> comp = body->parentComponent();
      if (comp) {
        LOG_DEBUG("getComponentFromEntity: Found component from BRepBody: " << comp->name());
        return comp;
      }
    }

    // Handle MeshBody -> parentComponent
    if (Ptr<adsk::fusion::MeshBody> meshBody = entity) {
      Ptr<adsk::fusion::Component> comp = meshBody->parentComponent();
      if (comp) {
        LOG_DEBUG("getComponentFromEntity: Found component from MeshBody: " << comp->name());
        return comp;
      }
    }

    // Handle Profile -> parentSketch -> parentComponent
    if (Ptr<adsk::fusion::Profile> profile = entity) {
      Ptr<adsk::fusion::Sketch> sketch = profile->parentSketch();
      if (sketch) {
        Ptr<adsk::fusion::Component> comp = sketch->parentComponent();
        if (comp) {
          LOG_DEBUG("getComponentFromEntity: Found component from Profile: " << comp->name());
          return comp;
        }
      }
    }

    // Handle Sketch -> parentComponent
    if (Ptr<adsk::fusion::Sketch> sketch = entity) {
      Ptr<adsk::fusion::Component> comp = sketch->parentComponent();
      if (comp) {
        LOG_DEBUG("getComponentFromEntity: Found component from Sketch: " << comp->name());
        return comp;
      }
    }

    // Handle ConstructionPlane -> component (note: uses .component() not .parentComponent())
    if (Ptr<adsk::fusion::ConstructionPlane> plane = entity) {
      Ptr<adsk::fusion::Component> comp = plane->component();
      if (comp) {
        LOG_DEBUG("getComponentFromEntity: Found component from ConstructionPlane: " << comp->name());
        return comp;
      }
    }

    LOG_WARNING("getComponentFromEntity: Could not determine component for entity type: " << entityType);
    return nullptr;
  } catch (const std::exception& e) {
    LOG_ERROR("getComponentFromEntity exception: " << e.what());
    return nullptr;
  } catch (...) {
    LOG_ERROR("getComponentFromEntity: Unknown exception");
    return nullptr;
  }
}

void FusionWorkspace::logApiError(const std::string& operation) const {
  if (!app_) {
    LOG_ERROR(operation << " failed: No Fusion application instance");
    return;
  }

  // Use Fusion's getLastError() for detailed diagnostic information
  // This is the recommended way to get error details per Fusion API docs
  std::string errorDescription;
  int errorCode = app_->getLastError(&errorDescription);

  if (errorCode != 0) {
    LOG_ERROR(operation << " failed - Fusion error " << errorCode << ": " << errorDescription);
  } else {
    // No specific error code - the operation just returned null/false
    LOG_ERROR(operation << " failed (no Fusion error code available)");
  }
}

}  // namespace Adapters
}  // namespace ChipCarving

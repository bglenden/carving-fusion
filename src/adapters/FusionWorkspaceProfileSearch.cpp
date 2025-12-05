/**
 * FusionWorkspaceProfileSearch.cpp
 *
 * Profile search and discovery operations for FusionWorkspace
 *
 * REFACTORED: Now uses Design.findEntityByToken() for direct profile lookup
 * instead of manually iterating through all components/sketches/profiles.
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

Ptr<adsk::fusion::Profile> FusionWorkspace::findProfileByEntityToken(const std::string& entityId) {
  LOG_INFO("Finding profile by entity token: " << entityId);

  if (entityId.empty()) {
    LOG_ERROR("findProfileByEntityToken called with empty entity ID");
    return nullptr;
  }

  if (!app_) {
    LOG_ERROR("No Fusion 360 application instance");
    return nullptr;
  }

  // Get the active design
  Ptr<adsk::fusion::Design> design = app_->activeProduct();
  if (!design) {
    LOG_ERROR("No active design");
    return nullptr;
  }

  // ========================================================================
  // DIRECT ENTITY LOOKUP using Design.findEntityByToken()
  // This replaces ~150 lines of manual iteration through components/sketches/profiles
  // ========================================================================

  LOG_DEBUG("Using direct entity lookup via findEntityByToken");

  std::vector<Ptr<Base>> entities = findEntitiesByToken(entityId);

  if (!entities.empty()) {
    // Try to cast to Profile
    Ptr<adsk::fusion::Profile> profile = entities[0];
    if (profile) {
      LOG_INFO("FOUND profile via direct lookup!");

      // Log additional info for debugging
      Ptr<adsk::fusion::Sketch> parentSketch = profile->parentSketch();
      if (parentSketch) {
        LOG_DEBUG("Profile is in sketch: " << parentSketch->name());
        Ptr<adsk::fusion::Component> comp = parentSketch->parentComponent();
        if (comp) {
          LOG_DEBUG("Sketch is in component: " << comp->name());
        }
      }

      return profile;
    }
    LOG_WARNING("Entity found but is not a Profile type. Actual type: " << entities[0]->objectType());
  }

  // ========================================================================
  // FALLBACK: If direct lookup fails (e.g., stale token), try alternative approaches
  // This preserves robustness for edge cases the old code handled
  // ========================================================================

  LOG_WARNING("Direct lookup failed. Attempting fallback strategies...");

  Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
  if (!rootComp) {
    LOG_ERROR("No root component for fallback");
    return nullptr;
  }

  // Build list of all components
  std::vector<Ptr<adsk::fusion::Component>> allComponents;
  allComponents.push_back(rootComp);

  auto occurrences = rootComp->allOccurrences();
  if (occurrences) {
    for (size_t i = 0; i < occurrences->count(); ++i) {
      auto occurrence = occurrences->item(i);
      if (!occurrence || !occurrence->isValid()) {
        LOG_DEBUG("Skipping invalid occurrence at index " << i);
        continue;
      }
      if (occurrence->component()) {
        allComponents.push_back(occurrence->component());
      }
    }
  }

  LOG_DEBUG("Fallback: Searching " << allComponents.size() << " components");

  // Fallback strategy: Look for a sketch with exactly one profile
  // This handles cases where the token became stale but there's an obvious choice
  for (size_t compIdx = 0; compIdx < allComponents.size(); ++compIdx) {
    auto component = allComponents[compIdx];
    if (!component || !component->isValid())
      continue;

    auto sketches = component->sketches();
    if (!sketches)
      continue;

    for (size_t i = 0; i < sketches->count(); ++i) {
      Ptr<adsk::fusion::Sketch> candidateSketch = sketches->item(i);
      if (!candidateSketch || !candidateSketch->isValid()) {
        LOG_DEBUG("Skipping invalid sketch at index " << i);
        continue;
      }

      Ptr<adsk::fusion::Profiles> profiles = candidateSketch->profiles();
      if (!profiles)
        continue;

      // If this sketch has exactly one profile, it might be what we're looking for
      if (profiles->count() == 1) {
        Ptr<adsk::fusion::Profile> profile = profiles->item(0);
        if (profile && profile->isValid()) {
          LOG_WARNING("Using fallback: single profile from sketch '" << candidateSketch->name() << "'");
          LOG_WARNING("Original token: " << entityId);
          LOG_WARNING("Fallback token: " << profile->entityToken());
          return profile;
        }
      }
    }
  }

  LOG_ERROR("Could not find profile with entity token: " << entityId);
  LOG_ERROR("Neither direct lookup nor fallback strategies succeeded");
  return nullptr;
}

}  // namespace Adapters
}  // namespace ChipCarving

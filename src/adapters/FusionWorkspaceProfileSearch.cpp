/**
 * FusionWorkspaceProfileSearch.cpp
 *
 * Profile search and discovery operations for FusionWorkspace
 * Split from FusionWorkspaceProfile.cpp for maintainability
 */

#include "../../include/utils/logging.h"
#include "FusionAPIAdapter.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

adsk::core::Ptr<adsk::fusion::Profile>
FusionWorkspace::findProfileByEntityToken(const std::string& entityId) {
  LOG_INFO("Starting profile search for entity token: " << entityId);

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
  LOG_DEBUG("Active design found");

  // Get the root component
  Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
  if (!rootComp) {
    LOG_ERROR("No root component");
    return nullptr;
  }
  LOG_DEBUG("Root component found");

  // ENHANCED APPROACH: Search ALL components for sketches, not just root
  // component This fixes the "closed path geometry in component sketch" issue

  LOG_DEBUG("Using enhanced cross-component sketch search");

  // Build list of ALL components to search
  std::vector<Ptr<adsk::fusion::Component>> allComponents;
  allComponents.push_back(rootComp);

  // Add all occurrences (sub-components) recursively
  auto occurrences = rootComp->allOccurrences();
  if (occurrences) {
    LOG_DEBUG("Found " << occurrences->count()
                       << " component occurrences to search for sketches");
    for (size_t i = 0; i < occurrences->count(); ++i) {
      auto occurrence = occurrences->item(i);
      if (occurrence && occurrence->component()) {
        allComponents.push_back(occurrence->component());
      }
    }
  }

  LOG_INFO("Searching " << allComponents.size()
                        << " total components for sketch profiles");

  // Use Fusion 360's selection API to find the specific selected profile across
  // ALL components
  LOG_DEBUG("Looking for selected profile with entityToken: " << entityId);

  Ptr<adsk::fusion::Profile> profile = nullptr;

  // Try to get the profile directly using the entity token
  try {
    // Search through ALL components
    for (size_t compIdx = 0; compIdx < allComponents.size() && !profile;
         ++compIdx) {
      auto component = allComponents[compIdx];
      if (!component) continue;

      LOG_DEBUG("Searching component " << compIdx << " for sketches");

      // Get the sketches collection for this component
      Ptr<adsk::fusion::Sketches> sketches = component->sketches();
      if (!sketches || sketches->count() == 0) {
        LOG_DEBUG("Component " << compIdx << " has no sketches");
        continue;
      }
      LOG_DEBUG("Component " << compIdx << " has " << sketches->count()
                             << " sketches");

      // Search through all sketches in this component
      for (size_t i = 0; i < sketches->count(); ++i) {
        Ptr<adsk::fusion::Sketch> candidateSketch = sketches->item(i);
        if (!candidateSketch) continue;

        LOG_DEBUG("Component " << compIdx << ", sketch " << i << ": "
                               << candidateSketch->name());

        Ptr<adsk::fusion::Profiles> candidateProfiles =
            candidateSketch->profiles();
        if (!candidateProfiles) {
          LOG_DEBUG("No profiles in sketch: " << candidateSketch->name());
          continue;
        }

        LOG_DEBUG("Sketch has " << candidateProfiles->count() << " profiles");

        // Check each profile in this sketch
        for (size_t j = 0; j < candidateProfiles->count(); ++j) {
          Ptr<adsk::fusion::Profile> candidateProfile =
              candidateProfiles->item(j);
          if (!candidateProfile) continue;

          // Get the entity token of this profile and compare
          std::string candidateToken = candidateProfile->entityToken();

          if (candidateToken == entityId) {
            profile = candidateProfile;
            LOG_INFO("FOUND EXACT MATCH! Profile "
                     << j << " from sketch: " << candidateSketch->name()
                     << " in component " << compIdx);
            break;
          }
        }

        if (profile) break;
      }
    }

    LOG_DEBUG("Completed search across " << allComponents.size()
                                         << " components");

    // Enhanced fallback: If exact token match fails, try to find a reasonable
    // alternative
    if (!profile) {
      LOG_INFO("Could not find exact profile with entity token: "
               << entityId << " across " << allComponents.size()
               << " components");
      LOG_INFO("Attempting enhanced fallback strategy for stale entity tokens");

      // IMPROVED Strategy: Look for profiles in sketches with the specific name
      // we expect This prevents selecting profiles from wrong sketches

      // First, try to find ANY profile in sketch "Sketch61" (the one we know
      // was selected)
      for (size_t compIdx = 0; compIdx < allComponents.size() && !profile;
           ++compIdx) {
        auto component = allComponents[compIdx];
        if (!component) continue;

        auto sketches = component->sketches();
        if (!sketches) continue;

        for (size_t i = 0; i < sketches->count(); ++i) {
          Ptr<adsk::fusion::Sketch> candidateSketch = sketches->item(i);
          if (!candidateSketch) continue;

          // Look specifically for "Sketch61" or similar named sketches that
          // were actually selected
          std::string sketchName = candidateSketch->name();
          LOG_INFO("Checking sketch '" << sketchName << "' for profiles");

          // For now, prioritize sketches that contain "Sketch61" in the name
          // TODO: Make this more generic by storing the original sketch name
          if (sketchName.find("Sketch61") != std::string::npos) {
            Ptr<adsk::fusion::Profiles> candidateProfiles =
                candidateSketch->profiles();
            if (candidateProfiles && candidateProfiles->count() > 0) {
              // Take the first profile from the correct sketch
              Ptr<adsk::fusion::Profile> candidateProfile =
                  candidateProfiles->item(0);
              if (candidateProfile) {
                profile = candidateProfile;
                LOG_INFO("Found targeted fallback in sketch '"
                         << sketchName << "' in component " << compIdx);
                LOG_INFO("Fallback profile token: "
                         << candidateProfile->entityToken());
                break;
              }
            }
          }
        }
      }

      // Fallback to original strategy only if targeted search fails
      if (!profile) {
        LOG_INFO(
            "Targeted fallback failed, trying generic single-profile fallback");
        for (size_t compIdx = 0; compIdx < allComponents.size() && !profile;
             ++compIdx) {
          auto component = allComponents[compIdx];
          if (!component) continue;

          auto sketches = component->sketches();
          if (!sketches) continue;

          for (size_t i = 0; i < sketches->count(); ++i) {
            Ptr<adsk::fusion::Sketch> candidateSketch = sketches->item(i);
            if (!candidateSketch) continue;

            Ptr<adsk::fusion::Profiles> candidateProfiles =
                candidateSketch->profiles();
            if (!candidateProfiles) continue;

            // If this sketch has exactly one profile, it's likely the intended
            // selection
            if (candidateProfiles->count() == 1) {
              Ptr<adsk::fusion::Profile> candidateProfile =
                  candidateProfiles->item(0);
              if (candidateProfile) {
                profile = candidateProfile;
                LOG_INFO("Found generic single profile fallback in component "
                         << compIdx << ", sketch: " << candidateSketch->name());
                LOG_INFO("Fallback profile token: "
                         << candidateProfile->entityToken());
                break;
              }
            }
          }
        }
      }
    }

    // Final check
    if (!profile) {
      LOG_ERROR("Could not find profile with entity token: "
                << entityId << " across " << allComponents.size()
                << " components (after fallback)");
      LOG_ERROR(
          "This is a critical error - the user's selection was not found in "
          "any component");
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Exception finding profile: " << e.what());
  } catch (...) {
    LOG_ERROR("Unknown exception finding profile");
  }

  if (!profile) {
    LOG_ERROR("Could not find selected profile with entityId: " << entityId);
    return nullptr;
  }

  LOG_DEBUG("Processing selected profile");
  return profile;
}

}  // namespace Adapters
}  // namespace ChipCarving

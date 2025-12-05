/**
 * PluginCommandsParametersSelection.cpp
 *
 * Selection input processing for PluginCommands
 * Split from PluginCommandsParameters.cpp for maintainability
 */

#include "PluginCommands.h"
#include "utils/logging.h"

namespace ChipCarving {
namespace Commands {

Adapters::SketchSelection GeneratePathsCommandHandler::getSelectionFromInputs(
    adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
  Adapters::SketchSelection selection;

  // IMMEDIATE EXTRACTION APPROACH: Use cached geometry instead of processing
  // selections
  LOG_INFO("Using cached geometry from immediate extraction. Available cached "
           "profiles: "
           << cachedProfiles_.size());

  if (!cachedProfiles_.empty()) {
    // Use the cached geometry that was extracted immediately when selections
    // were made
    selection.isValid = true;
    selection.closedPathCount = cachedProfiles_.size();
    selection.selectedProfiles = cachedProfiles_;

    // Generate dummy entity IDs for backward compatibility (these won't be
    // used)
    for (size_t i = 0; i < cachedProfiles_.size(); ++i) {
      selection.selectedEntityIds.push_back("cached_profile_" + std::to_string(i));
    }

    LOG_INFO("Successfully using " << cachedProfiles_.size() << " cached profiles");
    return selection;
  }

  // FALLBACK: If no cached geometry, try the original approach
  LOG_INFO("No cached geometry available, falling back to original selection "
           "processing");

  adsk::core::Ptr<adsk::core::SelectionCommandInput> profileSelection = inputs->itemById("sketchProfiles");
  if (profileSelection) {
    selection.closedPathCount = 0;  // Reset and count valid profiles only

    // First, separate selections into profiles and curves
    std::vector<adsk::core::Ptr<adsk::fusion::Profile>> directProfiles;
    std::map<std::string, std::vector<adsk::core::Ptr<adsk::fusion::SketchCurve>>> curvesBySketch;

    // Categorize each selected entity
    LOG_INFO("Processing " << profileSelection->selectionCount() << " selected entities");
    for (int i = 0; i < static_cast<int>(profileSelection->selectionCount()); i++) {
      adsk::core::Ptr<adsk::core::Base> entity = profileSelection->selection(i)->entity();
      if (entity) {
        std::string entityType = entity->objectType();
        // Try to cast as Profile first (for root component selections)
        auto profile = entity->cast<adsk::fusion::Profile>();
        if (profile) {
          LOG_INFO("Selection " << i << ": Profile from sketch '"
                                << (profile->parentSketch() ? profile->parentSketch()->name() : "unknown") << "'");
          directProfiles.push_back(profile);
        } else {
          // Try to cast as SketchCurve (for sub-component selections)
          auto sketchCurve = entity->cast<adsk::fusion::SketchCurve>();
          if (sketchCurve && sketchCurve->parentSketch()) {
            std::string sketchName = sketchCurve->parentSketch()->name();
            std::string sketchId = sketchCurve->parentSketch()->entityToken();
            LOG_INFO("Selection " << i << ": " << entityType << " from sketch '" << sketchName << "'");
            curvesBySketch[sketchId].push_back(sketchCurve);
          } else {
            LOG_INFO("Selection " << i << ": Unhandled entity type '" << entityType << "'");
          }
        }
      }
    }

    // Process direct profile selections (root component)
    for (const auto& profile : directProfiles) {
      // Use areaProperties() as validation - only works for closed profiles
      adsk::core::Ptr<adsk::fusion::AreaProperties> areaProps = profile->areaProperties();
      if (areaProps && areaProps->area() > 0) {
        // Valid closed profile with measurable area
        selection.closedPathCount++;

        // Store the entity token for backward compatibility
        std::string profileToken = profile->entityToken();
        selection.selectedEntityIds.push_back(profileToken);

        // Store profile metadata immediately INCLUDING the actual profile object
        Adapters::ProfileGeometry profileGeom;

        // Store basic profile data
        profileGeom.area = areaProps->area();
        auto centroid = areaProps->centroid();
        if (centroid) {
          profileGeom.centroid = {centroid->x(), centroid->y()};
        }

        // Get sketch name if available
        if (profile->parentSketch()) {
          profileGeom.sketchName = profile->parentSketch()->name();

          // Get plane entity ID
          auto referenceEntity = profile->parentSketch()->referencePlane();
          if (referenceEntity) {
            auto constructionPlane = referenceEntity->cast<adsk::fusion::ConstructionPlane>();
            if (constructionPlane) {
              profileGeom.planeEntityId = constructionPlane->entityToken();
            } else {
              auto face = referenceEntity->cast<adsk::fusion::BRepFace>();
              if (face) {
                profileGeom.planeEntityId = face->entityToken();
              }
            }
          }
        }

        // Add to selected profiles
        selection.selectedProfiles.push_back(profileGeom);
      } else {
        selection.errorMessage = "Selected profile has no area (not closed)";
        selection.isValid = false;
        return selection;
      }
    }

    // Process curve selections (sub-components)
    // Check if curves from each sketch form a closed loop
    LOG_INFO("Processing curves from " << curvesBySketch.size() << " sketches");
    for (const auto& sketchCurvePair : curvesBySketch) {
      const auto& curves = sketchCurvePair.second;
      if (curves.empty())
        continue;

      // Get the parent sketch from the first curve
      auto sketch = curves[0]->parentSketch();
      if (!sketch)
        continue;

      LOG_INFO("Checking " << curves.size() << " curves from sketch '" << sketch->name() << "' for complete profiles");

      // Check if these curves form a closed loop
      bool foundCompleteProfile = false;

      // Get all profiles in this sketch
      auto sketchProfiles = sketch->profiles();
      if (sketchProfiles) {
        LOG_INFO("Sketch has " << sketchProfiles->count() << " profiles");
        for (size_t p = 0; p < sketchProfiles->count(); ++p) {
          auto candidateProfile = sketchProfiles->item(p);
          if (!candidateProfile)
            continue;

          // Count how many curves from this profile are selected
          int matchingCurves = 0;
          int totalCurvesInProfile = 0;

          // Check each loop in the profile
          auto profileLoops = candidateProfile->profileLoops();
          if (profileLoops) {
            for (size_t l = 0; l < profileLoops->count(); ++l) {
              auto loop = profileLoops->item(l);
              if (!loop)
                continue;

              auto loopCurves = loop->profileCurves();
              if (loopCurves) {
                totalCurvesInProfile += loopCurves->count();

                // Check if selected curves match this loop's curves
                for (size_t c = 0; c < loopCurves->count(); ++c) {
                  auto profileCurve = loopCurves->item(c);
                  if (profileCurve && profileCurve->sketchEntity()) {
                    auto curveEntity = profileCurve->sketchEntity();

                    // Check if this curve is in our selected curves
                    for (const auto& selectedCurve : curves) {
                      if (curveEntity->entityToken() == selectedCurve->entityToken()) {
                        matchingCurves++;
                        break;
                      }
                    }
                  }
                }
              }
            }
          }

          LOG_INFO("Profile " << p << ": " << matchingCurves << "/" << totalCurvesInProfile
                              << " curves match selection");

          // If all curves from this profile are selected, we have a complete profile
          if (matchingCurves == totalCurvesInProfile && totalCurvesInProfile > 0) {
            LOG_INFO("Found complete profile match!");
            // We found a complete profile! Process it like a direct profile selection
            adsk::core::Ptr<adsk::fusion::AreaProperties> areaProps = candidateProfile->areaProperties();
            if (areaProps && areaProps->area() > 0) {
              selection.closedPathCount++;

              std::string profileToken = candidateProfile->entityToken();
              selection.selectedEntityIds.push_back(profileToken);

              Adapters::ProfileGeometry profileGeom;
              profileGeom.area = areaProps->area();
              auto centroid = areaProps->centroid();
              if (centroid) {
                profileGeom.centroid = {centroid->x(), centroid->y()};
              }

              profileGeom.sketchName = sketch->name();

              auto referenceEntity = sketch->referencePlane();
              if (referenceEntity) {
                auto constructionPlane = referenceEntity->cast<adsk::fusion::ConstructionPlane>();
                if (constructionPlane) {
                  profileGeom.planeEntityId = constructionPlane->entityToken();
                } else {
                  auto face = referenceEntity->cast<adsk::fusion::BRepFace>();
                  if (face) {
                    profileGeom.planeEntityId = face->entityToken();
                  }
                }
              }

              selection.selectedProfiles.push_back(profileGeom);
              foundCompleteProfile = true;
              break;
            }
          }
        }
      }

      if (!foundCompleteProfile) {
        // The selected curves don't form a complete profile
        selection.errorMessage = "Selected curves do not form a complete closed profile. "
                                 "For sub-components, you must select ALL curves that form the "
                                 "profile.";
        selection.isValid = false;
        return selection;
      }
    }

    // Final validation
    LOG_INFO("Total valid profiles found: " << selection.closedPathCount);
    selection.isValid = (selection.closedPathCount > 0);
    if (!selection.isValid) {
      if (profileSelection->selectionCount() > 0) {
        selection.errorMessage = "No valid closed profiles found. Ensure you click "
                                 "INSIDE blue shaded regions, not on curve edges.";
      } else {
        selection.errorMessage = "No closed profiles selected";
      }
    }
  } else {
    selection.errorMessage = "Profile selection input not found";
  }

  return selection;
}

}  // namespace Commands
}  // namespace ChipCarving

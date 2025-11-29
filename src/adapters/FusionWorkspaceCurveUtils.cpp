/**
 * FusionWorkspaceCurveUtils.cpp
 *
 * Plane extraction and sketch utility operations for FusionWorkspace
 * Split from FusionWorkspaceCurve.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#include "utils/logging.h"
#include "FusionAPIAdapter.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

std::string FusionWorkspace::extractPlaneEntityIdFromProfile(const std::string& profileEntityId) {
  LOG_DEBUG("extractPlaneEntityIdFromProfile called with profileEntityId: " << profileEntityId);

  try {
    if (!app_) {
      LOG_ERROR("No Fusion 360 application instance");
      return "";
    }

    // Get the active design
    Ptr<adsk::fusion::Design> design = app_->activeProduct();
    if (!design) {
      LOG_ERROR("No active design");
      return "";
    }

    // Get the root component
    Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
    if (!rootComp) {
      LOG_ERROR("No root component");
      return "";
    }

    // Find the profile using the entity ID
    LOG_DEBUG("Searching for profile with token: " << profileEntityId);

    // Search through all sketches to find the profile
    Ptr<adsk::fusion::Sketches> sketches = rootComp->sketches();
    if (!sketches) {
      LOG_ERROR("No sketches collection");
      return "";
    }

    for (int sketchIndex = 0; sketchIndex < static_cast<int>(sketches->count()); ++sketchIndex) {
      Ptr<adsk::fusion::Sketch> sketch = sketches->item(sketchIndex);
      if (!sketch)
        continue;

      LOG_DEBUG("Checking sketch " << sketchIndex << ": " << sketch->name());

      // Check profiles in this sketch
      Ptr<adsk::fusion::Profiles> profiles = sketch->profiles();
      if (!profiles)
        continue;

      for (int profileIndex = 0; profileIndex < static_cast<int>(profiles->count()); ++profileIndex) {
        Ptr<adsk::fusion::Profile> profile = profiles->item(profileIndex);
        if (!profile)
          continue;

        std::string currentProfileId = profile->entityToken();
        LOG_DEBUG("Profile " << profileIndex << " token: " << currentProfileId);

        if (currentProfileId == profileEntityId) {
          // Found the profile! Get its sketch's plane
          LOG_DEBUG("Found matching profile in sketch: " << sketch->name());

          // Get the sketch's reference plane
          Ptr<adsk::core::Base> referenceEntity = sketch->referencePlane();
          if (referenceEntity) {
            // Try to cast to construction plane
            Ptr<adsk::fusion::ConstructionPlane> constructionPlane = referenceEntity;
            if (constructionPlane) {
              std::string planeToken = constructionPlane->entityToken();
              LOG_DEBUG("Extracted construction plane token: " << planeToken);
              return planeToken;
            }

            // Try to cast to B-Rep face (if sketch is on a face)
            Ptr<adsk::fusion::BRepFace> face = referenceEntity;
            if (face) {
              std::string faceToken = face->entityToken();
              LOG_DEBUG("Extracted face plane token: " << faceToken);
              return faceToken;
            }

            LOG_WARNING("Reference plane found but couldn't extract entity token");
          } else {
            LOG_WARNING("Profile's sketch has no reference plane");
          }

          // If we can't get the plane token, return empty string but log the
          // attempt
          LOG_WARNING("Could not extract plane entity ID from profile's sketch");
          return "";
        }
      }
    }

    LOG_DEBUG("Profile with token '" << profileEntityId << "' not found in any sketch");
    return "";
  } catch (const std::exception& e) {
    LOG_ERROR("Exception in extractPlaneEntityIdFromProfile: " << e.what());
    return "";
  } catch (...) {
    LOG_ERROR("Unknown exception in extractPlaneEntityIdFromProfile");
    return "";
  }
}

std::vector<std::string> FusionWorkspace::getAllSketchNames() {
  std::vector<std::string> sketchNames;

  try {
    // Get the active design
    auto design = app_->activeProduct();
    if (!design) {
      return sketchNames;
    }

    auto fusionDesign = design->cast<adsk::fusion::Design>();
    if (!fusionDesign) {
      return sketchNames;
    }

    // Get the root component
    auto rootComp = fusionDesign->rootComponent();
    if (!rootComp) {
      return sketchNames;
    }

    // Get all sketches
    auto sketches = rootComp->sketches();
    if (!sketches) {
      return sketchNames;
    }

    // Iterate through all sketches and collect names
    for (size_t i = 0; i < sketches->count(); ++i) {
      auto sketch = sketches->item(i);
      if (sketch) {
        std::string sketchName = sketch->name();
        if (!sketchName.empty()) {
          sketchNames.push_back(sketchName);
        }
      }
    }
  } catch (const std::exception& e) {
    // Return empty vector on error
  } catch (...) {
    // Return empty vector on unknown error
  }

  return sketchNames;
}

}  // namespace Adapters
}  // namespace ChipCarving

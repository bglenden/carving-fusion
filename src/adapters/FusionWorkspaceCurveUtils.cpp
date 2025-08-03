/**
 * FusionWorkspaceCurveUtils.cpp
 *
 * Plane extraction and sketch utility operations for FusionWorkspace
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

std::string FusionWorkspace::extractPlaneEntityIdFromProfile(const std::string& profileEntityId) {
    // Use the main debug log
    std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
    std::ofstream debugLog(debugLogPath, std::ios::app);
    debugLog << "[INFO] extractPlaneEntityIdFromProfile called with profileEntityId: " << profileEntityId << std::endl;

    try {
        if (!app_) {
            debugLog << "[ERROR] No Fusion 360 application instance" << std::endl;
            return "";
        }

        // Get the active design
        Ptr<adsk::fusion::Design> design = app_->activeProduct();
        if (!design) {
            debugLog << "[ERROR] No active design" << std::endl;
            return "";
        }

        // Get the root component
        Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
        if (!rootComp) {
            debugLog << "[ERROR] No root component" << std::endl;
            return "";
        }

        // Find the profile using the entity ID
        debugLog << "[DEBUG] Searching for profile with token: " << profileEntityId << std::endl;

        // Search through all sketches to find the profile
        Ptr<adsk::fusion::Sketches> sketches = rootComp->sketches();
        if (!sketches) {
            debugLog << "[ERROR] No sketches collection" << std::endl;
            return "";
        }

        for (int sketchIndex = 0; sketchIndex < static_cast<int>(sketches->count()); ++sketchIndex) {
            Ptr<adsk::fusion::Sketch> sketch = sketches->item(sketchIndex);
            if (!sketch) continue;

            debugLog << "[DEBUG] Checking sketch " << sketchIndex << ": " << sketch->name() << std::endl;

            // Check profiles in this sketch
            Ptr<adsk::fusion::Profiles> profiles = sketch->profiles();
            if (!profiles) continue;

            for (int profileIndex = 0; profileIndex < static_cast<int>(profiles->count()); ++profileIndex) {
                Ptr<adsk::fusion::Profile> profile = profiles->item(profileIndex);
                if (!profile) continue;

                std::string currentProfileId = profile->entityToken();
                debugLog << "[DEBUG] Profile " << profileIndex << " token: " << currentProfileId << std::endl;

                if (currentProfileId == profileEntityId) {
                    // Found the profile! Get its sketch's plane
                    debugLog << "[SUCCESS] Found matching profile in sketch: " << sketch->name() << std::endl;

                    // Get the sketch's reference plane
                    Ptr<adsk::core::Base> referenceEntity = sketch->referencePlane();
                    if (referenceEntity) {
                        // Try to cast to construction plane
                        Ptr<adsk::fusion::ConstructionPlane> constructionPlane = referenceEntity;
                        if (constructionPlane) {
                            std::string planeToken = constructionPlane->entityToken();
                            debugLog << "[SUCCESS] Extracted construction plane token: " << planeToken << std::endl;
                            return planeToken;
                        }

                        // Try to cast to B-Rep face (if sketch is on a face)
                        Ptr<adsk::fusion::BRepFace> face = referenceEntity;
                        if (face) {
                            std::string faceToken = face->entityToken();
                            debugLog << "[SUCCESS] Extracted face plane token: " << faceToken << std::endl;
                            return faceToken;
                        }

                        debugLog << "[WARNING] Reference plane found but couldn't extract entity token" << std::endl;
                    } else {
                        debugLog << "[WARNING] Profile's sketch has no reference plane" << std::endl;
                    }

                    // If we can't get the plane token, return empty string but log the attempt
                    debugLog << "[WARNING] Could not extract plane entity ID from profile's sketch" << std::endl;
                    return "";
                }
            }
        }

        debugLog << "[ERROR] Profile with token '" << profileEntityId << "' not found in any sketch" << std::endl;
        return "";

    } catch (const std::exception& e) {
        std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
        std::ofstream debugLog(debugLogPath, std::ios::app);
        debugLog << "[ERROR] Exception in extractPlaneEntityIdFromProfile: " << e.what() << std::endl;
        return "";
    } catch (...) {
        std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
        std::ofstream debugLog(debugLogPath, std::ios::app);
        debugLog << "[ERROR] Unknown exception in extractPlaneEntityIdFromProfile" << std::endl;
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

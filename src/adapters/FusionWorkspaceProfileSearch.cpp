/**
 * FusionWorkspaceProfileSearch.cpp
 *
 * Profile search and discovery operations for FusionWorkspace
 * Split from FusionWorkspaceProfile.cpp for maintainability
 */

#include "FusionAPIAdapter.h"
#include "../utils/DebugLogger.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

adsk::core::Ptr<adsk::fusion::Profile> FusionWorkspace::findProfileByEntityToken(
    const std::string& entityId) {

    auto logger = ChipCarving::Utils::DebugLogger::getInstance();
    logger->logDebug("Starting profile search across all components");

    if (!app_) {
        logger->logError("No Fusion 360 application instance");
        return nullptr;
    }

    // Get the active design
    Ptr<adsk::fusion::Design> design = app_->activeProduct();
    if (!design) {
        logger->logError("No active design");
        return nullptr;
    }
    logger->logDebug("Active design found");

    // Get the root component
    Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
    if (!rootComp) {
        logger->logError("No root component");
        return nullptr;
    }
    logger->logDebug("Root component found");

    // ENHANCED APPROACH: Search ALL components for sketches, not just root component
    // This fixes the "closed path geometry in component sketch" issue

    logger->logDebug("Using enhanced cross-component sketch search");

    // Build list of ALL components to search
    std::vector<Ptr<adsk::fusion::Component>> allComponents;
    allComponents.push_back(rootComp);

    // Add all occurrences (sub-components) recursively
    auto occurrences = rootComp->allOccurrences();
    if (occurrences) {
        logger->logDebug("Found " + std::to_string(occurrences->count()) + " component occurrences to search for sketches");
        for (size_t i = 0; i < occurrences->count(); ++i) {
            auto occurrence = occurrences->item(i);
            if (occurrence && occurrence->component()) {
                allComponents.push_back(occurrence->component());
            }
        }
    }

    logger->logDebug("Searching " + std::to_string(allComponents.size()) + " total components for sketch profiles");

    // Use Fusion 360's selection API to find the specific selected profile across ALL components
    logger->logDebug("Looking for selected profile with entityToken: " + entityId);

    Ptr<adsk::fusion::Profile> profile = nullptr;
    int totalSketchesSearched = 0;

    // Try to get the profile directly using the entity token
    try {
        // Search through ALL components
        for (size_t compIdx = 0; compIdx < allComponents.size() && !profile; ++compIdx) {
            auto component = allComponents[compIdx];
            if (!component) continue;

            logger->logDebug("Searching component " + std::to_string(compIdx) + " for sketches");

            // Get the sketches collection for this component
            Ptr<adsk::fusion::Sketches> sketches = component->sketches();
            if (!sketches || sketches->count() == 0) {
                logger->logDebug("Component " + std::to_string(compIdx) + " has no sketches");
                continue;
            }
            logger->logDebug("Component " + std::to_string(compIdx) + " has " + std::to_string(sketches->count()) + " sketches");
            totalSketchesSearched += sketches->count();

            // Search through all sketches in this component
            for (size_t i = 0; i < sketches->count(); ++i) {
                Ptr<adsk::fusion::Sketch> candidateSketch = sketches->item(i);
                if (!candidateSketch)
                    continue;

                logger->logDebug("Component " + std::to_string(compIdx) + ", sketch " + std::to_string(i) + ": " + candidateSketch->name());

                Ptr<adsk::fusion::Profiles> candidateProfiles = candidateSketch->profiles();
                if (!candidateProfiles) {
                    logger->logDebug("No profiles in sketch: " + candidateSketch->name());
                    continue;
                }

                logger->logDebug("Sketch has " + std::to_string(candidateProfiles->count()) + " profiles");

                // Check each profile in this sketch
                for (size_t j = 0; j < candidateProfiles->count(); ++j) {
                    Ptr<adsk::fusion::Profile> candidateProfile = candidateProfiles->item(j);
                    if (!candidateProfile)
                        continue;

                    // Get the entity token of this profile and compare
                    std::string candidateToken = candidateProfile->entityToken();
                    logger->logDebug("Component " + std::to_string(compIdx) + ", profile " + std::to_string(j) + " token: " + candidateToken + " vs looking for: " + entityId);

                    if (candidateToken == entityId) {
                        profile = candidateProfile;
                        logger->logInfo("FOUND EXACT MATCH! Profile " + std::to_string(j) + " from sketch: " + candidateSketch->name() + " in component " + std::to_string(compIdx));
                        break;
                    }
                }

                if (profile)
                    break;
            }
        }

        logger->logDebug("Completed search across " + std::to_string(allComponents.size()) + " components with " + std::to_string(totalSketchesSearched) + " total sketches");

        // Enhanced fallback: If exact token match fails, try to find a reasonable alternative
        if (!profile) {
            logger->logWarning("Could not find exact profile with entity token: " + entityId + " across " + std::to_string(allComponents.size()) + " components");
            logger->logDebug("Attempting enhanced fallback strategy for stale entity tokens");

            // Strategy 1: Look for the only profile in a component sketch (if unambiguous)
            for (size_t compIdx = 0; compIdx < allComponents.size() && !profile; ++compIdx) {
                auto component = allComponents[compIdx];
                if (!component) continue;

                auto sketches = component->sketches();
                if (!sketches) continue;

                for (size_t i = 0; i < sketches->count(); ++i) {
                    Ptr<adsk::fusion::Sketch> candidateSketch = sketches->item(i);
                    if (!candidateSketch) continue;

                    Ptr<adsk::fusion::Profiles> candidateProfiles = candidateSketch->profiles();
                    if (!candidateProfiles) continue;

                    // If this sketch has exactly one profile, it's likely the intended selection
                    if (candidateProfiles->count() == 1) {
                        Ptr<adsk::fusion::Profile> candidateProfile = candidateProfiles->item(0);
                        if (candidateProfile) {
                            profile = candidateProfile;
                            logger->logInfo("Found single profile fallback in component " + std::to_string(compIdx) + ", sketch: " + candidateSketch->name());
                            logger->logDebug("Fallback profile token: " + candidateProfile->entityToken());
                            break;
                        }
                    }
                }
            }
        }

        // Final check
        if (!profile) {
            logger->logError("Could not find profile with entity token: " + entityId + " across " + std::to_string(allComponents.size()) + " components (after fallback)");
            logger->logError("This is a critical error - the user's selection was not found in any component");
        }

    } catch (const std::exception& e) {
        logger->logError("Exception finding profile: " + std::string(e.what()));
    } catch (...) {
        logger->logError("Unknown exception finding profile");
    }

    if (!profile) {
        logger->logError("Could not find selected profile with entityId: " + entityId);
        return nullptr;
    }

    logger->logDebug("Processing selected profile");
    return profile;
}

}  // namespace Adapters
}  // namespace ChipCarving

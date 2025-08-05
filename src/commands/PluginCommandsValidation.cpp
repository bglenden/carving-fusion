/**
 * PluginCommandsValidation.cpp
 *
 * Selection validation for PluginCommands
 * Ensures only closed profiles can be selected
 */

#include "PluginCommands.h"
#include "../../include/utils/logging.h"
#include <algorithm>

namespace ChipCarving {
namespace Commands {

bool GeneratePathsCommandHandler::isPartOfClosedProfile(
    adsk::core::Ptr<adsk::fusion::SketchCurve> curve) {
    
    if (!curve || !curve->parentSketch()) {
        return false;
    }
    
    try {
        // Get the parent sketch
        auto sketch = curve->parentSketch();
        auto profiles = sketch->profiles();
        
        LOG_INFO("    Checking curve in sketch with " << (profiles ? profiles->count() : 0) << " profiles");
        
        if (!profiles || profiles->count() == 0) {
            return false;
        }
        
        // Check each profile in the sketch
        for (int p = 0; p < static_cast<int>(profiles->count()); ++p) {
            auto profile = profiles->item(p);
            if (!profile) continue;
            
            // Check if this profile is closed (has area)
            auto areaProps = profile->areaProperties();
            if (!areaProps || areaProps->area() <= 0) {
                LOG_INFO("      Profile " << p << " is not closed (no area)");
                continue;  // Not a closed profile
            }
            
            LOG_INFO("      Profile " << p << " has area: " << areaProps->area());
            
            // Check if the curve is part of this profile
            auto profileLoops = profile->profileLoops();
            if (!profileLoops) continue;
            
            for (int l = 0; l < profileLoops->count(); ++l) {
                auto loop = profileLoops->item(l);
                if (!loop) continue;
                
                auto loopCurves = loop->profileCurves();
                if (!loopCurves) continue;
                
                for (int c = 0; c < loopCurves->count(); ++c) {
                    auto profileCurve = loopCurves->item(c);
                    if (profileCurve && profileCurve->sketchEntity()) {
                        // Check if this is our curve
                        if (profileCurve->sketchEntity()->entityToken() == curve->entityToken()) {
                            return true;  // Found it in a closed profile
                        }
                    }
                }
            }
        }
        
        return false;  // Not found in any closed profile
        
    } catch (...) {
        LOG_INFO("Error checking if curve is part of closed profile");
        return false;
    }
}

void GeneratePathsCommandHandler::validateAndCleanSelection(
    adsk::core::Ptr<adsk::core::SelectionCommandInput> selectionInput) {
    
    if (!selectionInput) return;
    
    LOG_INFO("Validating " << selectionInput->selectionCount() << " selections...");
    
    // Track which curves belong to which profiles
    std::map<std::string, std::vector<adsk::core::Ptr<adsk::fusion::SketchCurve>>> curvesBySketch;
    std::vector<int> indicesToRemove;
    
    // Process selections from back to front to safely remove invalid ones
    for (int i = static_cast<int>(selectionInput->selectionCount()) - 1; i >= 0; --i) {
        auto selection = selectionInput->selection(i);
        if (!selection || !selection->entity()) {
            indicesToRemove.push_back(i);
            continue;
        }
        
        auto entity = selection->entity();
        std::string entityType = entity->objectType();
        LOG_INFO("  Selection " << i << " type: " << entityType);
        
        // If it's already a profile, it's valid
        if (entity->cast<adsk::fusion::Profile>()) {
            LOG_INFO("  Selection " << i << " is a valid profile");
            continue;
        }
        
        // If it's a curve, we need to track it to see if user selected ALL curves of a profile
        auto sketchCurve = entity->cast<adsk::fusion::SketchCurve>();
        if (sketchCurve) {
            // Log parent sketch info
            if (sketchCurve->parentSketch()) {
                LOG_INFO("    Parent sketch: " << sketchCurve->parentSketch()->name());
                std::string sketchId = sketchCurve->parentSketch()->entityToken();
                curvesBySketch[sketchId].push_back(sketchCurve);
            }
            
            // Mark individual curves for removal - we'll check later if they form complete profiles
            LOG_INFO("  Selection " << i << " is an individual curve - marking for potential removal");
            indicesToRemove.push_back(i);
        } else {
            // Unknown entity type - remove it
            LOG_INFO("  Removing selection " << i << ": Unknown entity type " << entity->objectType());
            indicesToRemove.push_back(i);
        }
    }
    
    // Check if any selected curves form complete profiles
    for (const auto& [sketchId, curves] : curvesBySketch) {
        if (!curves.empty() && curves[0]->parentSketch()) {
            auto sketch = curves[0]->parentSketch();
            auto profiles = sketch->profiles();
            
            if (profiles) {
                // Check each profile to see if we have all its curves selected
                for (int p = 0; p < static_cast<int>(profiles->count()); ++p) {
                    auto profile = profiles->item(p);
                    if (!profile) continue;
                    
                    // Only check closed profiles
                    auto areaProps = profile->areaProperties();
                    if (!areaProps || areaProps->area() <= 0) continue;
                    
                    // Count curves in this profile
                    int profileCurveCount = 0;
                    int matchingCurveCount = 0;
                    
                    auto profileLoops = profile->profileLoops();
                    if (profileLoops) {
                        for (int l = 0; l < profileLoops->count(); ++l) {
                            auto loop = profileLoops->item(l);
                            if (!loop) continue;
                            
                            auto loopCurves = loop->profileCurves();
                            if (loopCurves) {
                                profileCurveCount += loopCurves->count();
                                
                                // Check if our selected curves match
                                for (int c = 0; c < loopCurves->count(); ++c) {
                                    auto profileCurve = loopCurves->item(c);
                                    if (profileCurve && profileCurve->sketchEntity()) {
                                        std::string curveToken = profileCurve->sketchEntity()->entityToken();
                                        
                                        // Check if this curve is in our selection
                                        for (const auto& selectedCurve : curves) {
                                            if (selectedCurve->entityToken() == curveToken) {
                                                matchingCurveCount++;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // If we have ALL curves from this profile, remove them from indicesToRemove
                    if (matchingCurveCount == profileCurveCount && profileCurveCount > 0) {
                        LOG_INFO("  Found complete profile from " << matchingCurveCount << " selected curves");
                        
                        // Remove these curves from indicesToRemove
                        for (int i = static_cast<int>(selectionInput->selectionCount()) - 1; i >= 0; --i) {
                            auto selection = selectionInput->selection(i);
                            if (selection && selection->entity()) {
                                auto entity = selection->entity();
                                auto curve = entity->cast<adsk::fusion::SketchCurve>();
                                if (curve && curve->parentSketch() && 
                                    curve->parentSketch()->entityToken() == sketchId) {
                                    // This curve is part of the complete profile - remove from removal list
                                    indicesToRemove.erase(
                                        std::remove(indicesToRemove.begin(), indicesToRemove.end(), i),
                                        indicesToRemove.end());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // If we have invalid selections, we need to rebuild the selection list
    if (!indicesToRemove.empty()) {
        // Collect all valid selections first
        std::vector<adsk::core::Ptr<adsk::core::Base>> validSelections;
        
        for (int i = 0; i < static_cast<int>(selectionInput->selectionCount()); ++i) {
            // Skip if this index is marked for removal
            bool shouldRemove = false;
            for (int removeIdx : indicesToRemove) {
                if (i == removeIdx) {
                    shouldRemove = true;
                    break;
                }
            }
            
            if (!shouldRemove) {
                auto selection = selectionInput->selection(i);
                if (selection && selection->entity()) {
                    validSelections.push_back(selection->entity());
                }
            }
        }
        
        // Clear all selections and re-add only valid ones
        try {
            selectionInput->clearSelection();
            
            for (auto& validEntity : validSelections) {
                selectionInput->addSelection(validEntity);
            }
            
            LOG_INFO("Removed " << indicesToRemove.size() << " invalid selections. " 
                     << validSelections.size() << " valid selections remain.");
            
        } catch (...) {
            LOG_ERROR("Failed to update selection after validation");
        }
    }
    
    // Optional: Log a warning if user selected curves that don't form complete profiles
    for (const auto& [sketchId, curves] : curvesBySketch) {
        if (!curves.empty() && curves[0]->parentSketch()) {
            auto sketch = curves[0]->parentSketch();
            LOG_INFO("Sketch '" << sketch->name() << "' has " << curves.size() 
                     << " selected curves from closed profiles");
        }
    }
}

}  // namespace Commands
}  // namespace ChipCarving
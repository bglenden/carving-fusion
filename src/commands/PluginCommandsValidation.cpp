/**
 * PluginCommandsValidation.cpp
 *
 * Selection validation for PluginCommands
 * Ensures only closed profiles can be selected
 */

#include <algorithm>

#include "../../include/utils/logging.h"
#include "PluginCommands.h"

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

    LOG_INFO("    Checking curve in sketch with "
             << (profiles ? profiles->count() : 0) << " profiles");

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
            if (profileCurve->sketchEntity()->entityToken() ==
                curve->entityToken()) {
              return true;  // Found it in a closed profile
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

    for (int i = 0; i < static_cast<int>(selectionInput->selectionCount());
         ++i) {
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
                          << validSelections.size()
                          << " valid selections remain.");
    } catch (...) {
      LOG_ERROR("Failed to update selection after validation");
    }
  }

  // Optional: Log a warning if user selected curves that don't form complete
  // profiles
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
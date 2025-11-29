/**
 * PluginManagerLegacyPaths.cpp
 *
 * Legacy path generation functionality for PluginManager
 * Split from PluginManagerPaths.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>

#include "PluginManager.h"
#include "geometry/MedialAxisProcessor.h"
#include "geometry/Point2D.h"
#include "geometry/Point3D.h"

namespace ChipCarving {
namespace Core {

bool PluginManager::executeGeneratePaths() {
  if (!initialized_) {
    return false;
  }

  try {
    // Check if we have imported shapes
    if (importedShapes_.empty()) {
      std::string message = "OLD METHOD CALLED - No Design Imported\n\n";
      message += "This error means the OLD executeGeneratePaths() method\n";
      message += "is being called instead of the NEW executeMedialAxisGeneration()\n";
      message += "method. This should NOT happen if using the Enhanced UI.\n\n";
      message += "Please check which command you're clicking.";

      ui_->showMessageBox("OLD METHOD - Generate Paths", message);
      return true;  // Not an error, just nothing to process
    }

    // Create new sketch for medial axis visualization
    auto medialSketch = workspace_->createSketch("Medial Axis Paths");
    if (!medialSketch) {
      throw std::runtime_error("Failed to create medial axis sketch");
    }

    logger_->logInfo("Started legacy path generation for " + std::to_string(importedShapes_.size()) + " shapes");

    bool hasAnyResults = false;
    int processedShapes = 0;

    // Process each imported shape
    for (const auto& shape : importedShapes_) {
      try {
        logger_->logInfo("Processing shape " + std::to_string(processedShapes + 1));

        // Add shape outline to sketch first
        shape->drawToSketch(medialSketch.get(), logger_.get());

        // Generate and display medial axis for each shape
        Geometry::MedialAxisProcessor processor;
        auto results = processor.computeMedialAxis(*shape);

        if (results.success && !results.chains.empty()) {
          hasAnyResults = true;
          logger_->logInfo("Generated medial axis with " + std::to_string(results.chains.size()) +
                           " chains for shape " + std::to_string(processedShapes + 1));

          // Draw medial axis lines in the sketch
          for (const auto& chain : results.chains) {
            if (chain.size() < 2)
              continue;

            for (size_t i = 0; i < chain.size() - 1; ++i) {
              const auto& p1 = chain[i];
              const auto& p2 = chain[i + 1];

              // Add medial axis line to sketch (Point2D has x,y members)
              medialSketch->addLineToSketch(p1.x, p1.y, p2.x, p2.y);
            }
          }
        } else {
          logger_->logWarning("No medial axis generated for shape " + std::to_string(processedShapes + 1));
        }

        processedShapes++;
      } catch (const std::exception& e) {
        logger_->logError("Error processing shape " + std::to_string(processedShapes + 1) + ": " + e.what());
        // Continue with next shape
        processedShapes++;
      }
    }

    if (hasAnyResults) {
      ui_->showMessageBox("Legacy Paths Generated",
                          "Generated legacy medial axis paths for " + std::to_string(processedShapes) + " shapes.");
    } else {
      ui_->showMessageBox("No Results", "No medial axis paths were generated.");
    }

    logger_->logInfo("Legacy path generation completed for " + std::to_string(processedShapes) + " shapes");
    return true;
  } catch (const std::exception& e) {
    logger_->logError("Legacy path generation failed: " + std::string(e.what()));
    ui_->showMessageBox("Error", "Legacy path generation failed: " + std::string(e.what()));
    return false;
  }
}

}  // namespace Core
}  // namespace ChipCarving

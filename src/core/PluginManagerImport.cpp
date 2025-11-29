/**
 * PluginManagerImport.cpp
 *
 * Import design functionality for PluginManager
 * Split from PluginManager.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>

#include "PluginManager.h"
#include "geometry/Point2D.h"
#include "parsers/DesignParser.h"
#include "utils/logging.h"

namespace ChipCarving {
namespace Core {

bool PluginManager::executeImportDesign() {
  if (!initialized_) {
    return false;
  }

  try {
    auto totalStart = std::chrono::high_resolution_clock::now();

    // Get file selection from user
    std::string filePath = ui_->showFileDialog("Select Design File", "JSON Files (*.json)");

    if (filePath.empty()) {
      return true;  // User cancelled, not an error
    }

    // Read and parse the design file
    auto parseStart = std::chrono::high_resolution_clock::now();
    auto design = Parsers::DesignParser::parseFromFile(filePath, logger_.get());
    auto parseEnd = std::chrono::high_resolution_clock::now();
    auto parseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(parseEnd - parseStart);
    logger_->logInfo("⏱️ JSON parsing took: " + std::to_string(parseDuration.count()) + "ms");

    // Clear previous imports
    importedShapes_.clear();

    // Store shapes for medial axis processing
    for (auto& shape : design.shapes) {
      try {
        // Move shape to our storage (transfer ownership)
        importedShapes_.push_back(std::move(shape));
      } catch (const std::exception& e) {
        (void)e;  // Continue with other shapes
      }
    }

    // Store the file path for reference
    lastImportedFile_ = filePath;

    // Create sketch from stored shapes for visualization
    auto sketchStart = std::chrono::high_resolution_clock::now();
    auto sketch = workspace_->createSketch("Imported Design");
    if (!sketch) {
      throw std::runtime_error("Failed to create sketch in workspace");
    }

    // Add each stored shape to the sketch
    for (size_t i = 0; i < importedShapes_.size(); ++i) {
      const auto& shape = importedShapes_[i];
      try {
        auto shapeStart = std::chrono::high_resolution_clock::now();
        sketch->addShape(shape.get(), logger_.get());
        auto shapeEnd = std::chrono::high_resolution_clock::now();
        auto shapeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(shapeEnd - shapeStart);
        logger_->logInfo("⏱️ Shape " + std::to_string(i) + " drawing took: " + std::to_string(shapeDuration.count()) +
                         "ms");
      } catch (const std::exception& e) {
        (void)e;  // Continue with other shapes
      }
    }

    auto sketchEnd = std::chrono::high_resolution_clock::now();
    auto sketchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sketchEnd - sketchStart);
    logger_->logInfo("⏱️ Total sketch creation took: " + std::to_string(sketchDuration.count()) + "ms");

    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart);
    logger_->logInfo("⏱️ TOTAL IMPORT took: " + std::to_string(totalDuration.count()) + "ms");

    // Log completion (no popup)
    return true;
  } catch (const std::exception& e) {
    std::string errorMsg = "Failed to import design: " + std::string(e.what());
    ui_->showMessageBox("Import Design - Error", errorMsg);
    return false;
  } catch (...) {
    std::string errorMsg = "Unknown error during import";
    ui_->showMessageBox("Import Design - Error", errorMsg);
    return false;
  }
}

bool PluginManager::executeImportDesign(const std::string& filePath, const std::string& planeEntityId) {
  if (!initialized_) {
    return false;
  }

  try {
    if (filePath.empty()) {
      return false;
    }

    // Read and parse the design file
    auto design = Parsers::DesignParser::parseFromFile(filePath, logger_.get());

    // Clear previous imports
    importedShapes_.clear();

    // Store shapes for medial axis processing
    for (auto& shape : design.shapes) {
      try {
        // Move shape to our storage (transfer ownership)
        importedShapes_.push_back(std::move(shape));
      } catch (const std::exception& e) {
        (void)e;  // Continue with other shapes
      }
    }

    // Store the file path and plane entity ID for reference
    lastImportedFile_ = filePath;
    lastImportedPlaneEntityId_ = planeEntityId;

    // Debug logging
    LOG_DEBUG("Stored plane entity ID during import: '" << planeEntityId << "' (length: " << planeEntityId.length()
                                                        << ")");

    // Create sketch on specified plane or XY plane
    std::unique_ptr<Adapters::ISketch> sketch;
    if (!planeEntityId.empty()) {
      sketch = workspace_->createSketchOnPlane("Imported Design", planeEntityId);
    } else {
      sketch = workspace_->createSketch("Imported Design");
    }

    if (!sketch) {
      throw std::runtime_error("Failed to create sketch in workspace");
    }

    // Add each stored shape to the sketch
    for (size_t i = 0; i < importedShapes_.size(); ++i) {
      const auto& shape = importedShapes_[i];
      try {
        sketch->addShape(shape.get(), logger_.get());
      } catch (const std::exception& e) {
        (void)e;  // Continue with other shapes
      }
    }

    // Log completion (no popup)
    return true;
  } catch (const std::exception& e) {
    std::string errorMsg = "Failed to import design: " + std::string(e.what());
    ui_->showMessageBox("Import Design - Error", errorMsg);
    return false;
  } catch (...) {
    std::string errorMsg = "Unknown error during design import";
    ui_->showMessageBox("Import Design - Error", errorMsg);
    return false;
  }
}

}  // namespace Core
}  // namespace ChipCarving

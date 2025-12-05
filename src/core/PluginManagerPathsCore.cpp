/**
 * PluginManagerPathsCore.cpp
 *
 * Core path generation coordination and main execution logic for PluginManager
 * Split from PluginManagerPaths.cpp for maintainability
 */

#include <algorithm>
#include <chrono>

#include "PluginManager.h"
#include "geometry/Point2D.h"
#include "utils/logging.h"

namespace ChipCarving {
namespace Core {

bool PluginManager::executeMedialAxisGeneration(const Adapters::SketchSelection& selection,
                                                const Adapters::MedialAxisParameters& params) {
  if (!initialized_) {
    return false;
  }

  try {
    auto totalStart = std::chrono::high_resolution_clock::now();
    logger_->logInfo("⏱️ === GENERATE PATHS PROFILING STARTED ===");

    // Validate selection
    if (!selection.isValid || selection.closedPathCount == 0) {
      std::string errorMsg = "Invalid profile selection: " + selection.errorMessage;
      ui_->showMessageBox("Medial Axis Generation - Error", errorMsg);
      return false;
    }

    // Update medial processor parameters
    medialProcessor_->setPolygonTolerance(params.polygonTolerance);
    // Note: medialThreshold is not user-configurable via UI, uses processor
    // default

    // Try to extract plane information from selected profiles (needed for both
    // visualization and V-carve)
    std::string sourcePlaneId;
    if (!selection.selectedEntityIds.empty()) {
      // Use workspace to extract plane info from first selected profile
      sourcePlaneId = workspace_->extractPlaneEntityIdFromProfile(selection.selectedEntityIds[0]);
    }

    // Only create visualization sketch if generateVisualization is enabled
    std::unique_ptr<Adapters::ISketch> constructionSketch;
    std::string sketchName = "Medial Axis - " + params.toolName;  // Define here for result message

    if (params.generateVisualization) {
      // Create or find existing sketch for construction geometry visualization
      // Always create a new sketch on the correct plane (don't reuse old
      // sketches) Delete any existing sketch first to avoid conflicts
      auto existingSketch = workspace_->findSketch(sketchName);
      if (existingSketch) {
        existingSketch->clearConstructionGeometry();
      }

      // Create sketch in the component containing the target surface, or use
      // plane-based creation
      if (!params.targetSurfaceId.empty()) {
        // Target surface specified - create sketch in the component containing
        // the surface
        LOG_DEBUG("Creating construction sketch in target surface component: '" << params.targetSurfaceId << "'");

        constructionSketch = workspace_->createSketchInTargetComponent(sketchName, params.targetSurfaceId);
      } else if (!sourcePlaneId.empty()) {
        // Debug logging
        LOG_DEBUG("Using source plane entity ID for construction sketch: '"
                  << sourcePlaneId << "' (length: " << sourcePlaneId.length() << ")");

        constructionSketch = workspace_->createSketchOnPlane(sketchName, sourcePlaneId);
      } else if (!lastImportedPlaneEntityId_.empty()) {
        // Additional debug logging
        LOG_DEBUG("Using stored plane entity ID for construction sketch: '"
                  << lastImportedPlaneEntityId_ << "' (length: " << lastImportedPlaneEntityId_.length() << ")");

        constructionSketch = workspace_->createSketchOnPlane(sketchName, lastImportedPlaneEntityId_);
      } else {
        constructionSketch = workspace_->createSketch(sketchName);
      }

      if (!constructionSketch) {
        std::string errorMsg = "Failed to create construction geometry sketch";
        ui_->showMessageBox("Medial Axis Generation - Error", errorMsg);
        return false;
      }
    }

    // Enhanced UI Phase 5.2: Extract geometry from Fusion profiles
    auto extractionStart = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<Geometry::Point2D>> profilePolygons;
    std::vector<Adapters::IWorkspace::TransformParams> profileTransforms;
    bool extractionSuccess = extractProfileGeometry(selection, profilePolygons, profileTransforms);
    auto extractionEnd = std::chrono::high_resolution_clock::now();
    auto extractionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(extractionEnd - extractionStart);
    logger_->logInfo("⏱️ Profile geometry extraction took: " + std::to_string(extractionDuration.count()) + "ms");

    if (!extractionSuccess || profilePolygons.empty()) {
      LOG_INFO("Profile extraction failed or no polygons found");
      std::string errorMsg = "Failed to extract geometry from selected profiles.\nPlease "
                             "ensure valid closed sketch profiles are selected.";
      ui_->showMessageBox("Medial Axis Generation - Extraction Error", errorMsg);
      return false;
    }

    LOG_INFO("Starting medial axis computation for " << profilePolygons.size() << " profiles");

    std::vector<Geometry::MedialAxisResults> allResults;
    int successCount = 0;
    int totalPoints = 0;
    double totalLength = 0.0;

    // Process each extracted profile polygon
    auto allMedialStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < profilePolygons.size(); ++i) {
      const auto& polygon = profilePolygons[i];

      LOG_INFO("Processing profile " << i << " with " << polygon.size() << " vertices");

      try {
        // Log polygon bounds before medial axis computation
        if (!polygon.empty()) {
          double minX = polygon[0].x;
          double maxX = polygon[0].x;
          double minY = polygon[0].y;
          double maxY = polygon[0].y;
          for (const auto& pt : polygon) {
            minX = std::min(minX, pt.x);
            maxX = std::max(maxX, pt.x);
            minY = std::min(minY, pt.y);
            maxY = std::max(maxY, pt.y);
          }
          LOG_INFO("  Polygon bounds: (" << minX << ", " << minY << ") to (" << maxX << ", " << maxY << ")");
        }

        // Compute medial axis from polygon vertices using direct Point2D vector
        // method
        LOG_INFO("  Starting medial axis computation...");
        auto medialStart = std::chrono::high_resolution_clock::now();
        auto results = medialProcessor_->computeMedialAxis(polygon);
        auto medialEnd = std::chrono::high_resolution_clock::now();
        LOG_INFO("  Medial axis computation completed. Success: " << results.success);
        auto medialDuration = std::chrono::duration_cast<std::chrono::milliseconds>(medialEnd - medialStart);
        logger_->logInfo("⏱️ MedialAxis computation " + std::to_string(i) +
                         " took: " + std::to_string(medialDuration.count()) + "ms");

        if (results.success) {
          LOG_INFO("  Medial axis SUCCESS: " << results.chains.size() << " chains, " << results.totalPoints
                                             << " points, length=" << results.totalLength);
          // Log medial axis results bounds
          if (!results.chains.empty() && !results.chains[0].empty()) {
            // Find bounds of medial axis result
            // FIXED: Medial axis results are already in world coordinates (cm),
            // don't convert again
            double minX = results.chains[0][0].x;  // Already in cm
            double maxX = minX;
            double minY = results.chains[0][0].y;
            double maxY = minY;
            for (const auto& chain : results.chains) {
              for (const auto& pt : chain) {
                double x_cm = pt.x;  // Already in cm
                double y_cm = pt.y;  // Already in cm
                minX = std::min(minX, x_cm);
                maxX = std::max(maxX, x_cm);
                minY = std::min(minY, y_cm);
                maxY = std::max(maxY, y_cm);
              }
            }
          }

          allResults.push_back(results);
          successCount++;
          totalPoints += results.totalPoints;
          totalLength += results.totalLength;

          // Enhanced UI Phase 5.3: Add construction geometry visualization
          // Only add visualization if enabled
          if (params.generateVisualization) {
            auto vizStart = std::chrono::high_resolution_clock::now();
            // Use the corresponding transform for this profile
            if (i < profileTransforms.size()) {
              addConstructionGeometryVisualization(constructionSketch.get(), results, params, profileTransforms[i],
                                                   polygon);
            }
            auto vizEnd = std::chrono::high_resolution_clock::now();
            auto vizDuration = std::chrono::duration_cast<std::chrono::milliseconds>(vizEnd - vizStart);
            logger_->logInfo("⏱️ Shape " + std::to_string(i) +
                             " visualization took: " + std::to_string(vizDuration.count()) + "ms");
          }
        } else {
          LOG_ERROR("  Medial axis FAILED: " << results.errorMessage);
        }
      } catch (const std::exception& e) {
        LOG_ERROR("  Exception during medial axis processing: " << e.what());
      }
    }
    auto allMedialEnd = std::chrono::high_resolution_clock::now();
    auto allMedialDuration = std::chrono::duration_cast<std::chrono::milliseconds>(allMedialEnd - allMedialStart);
    logger_->logInfo("⏱️ All medial axis computations took: " + std::to_string(allMedialDuration.count()) + "ms");

    // Finalize construction geometry sketch if visualization is enabled
    auto visualizationStart = std::chrono::high_resolution_clock::now();
    if (params.generateVisualization && constructionSketch) {
      constructionSketch->finishSketch();
    }
    auto visualizationEnd = std::chrono::high_resolution_clock::now();
    auto visualizationDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(visualizationEnd - visualizationStart);
    logger_->logInfo("⏱️ Construction geometry visualization took: " + std::to_string(visualizationDuration.count()) +
                     "ms");

    // Generate V-carve toolpaths if enabled
    auto vcarveStart = std::chrono::high_resolution_clock::now();
    if (params.generateVCarveToolpaths && successCount > 0) {
      // Always create a new V-carve sketch on the correct plane
      std::string vcarveSketchName = "V-Carve Toolpaths - " + params.toolName;
      auto existingVcarveSketch = workspace_->findSketch(vcarveSketchName);
      if (existingVcarveSketch) {
        existingVcarveSketch->clearConstructionGeometry();
      }

      // Create V-carve sketch on same plane as source design
      std::unique_ptr<Adapters::ISketch> vcarveSketch;

      // Create V-carve sketch in the component containing the target surface,
      // or use plane-based creation
      if (!params.targetSurfaceId.empty()) {
        // Target surface specified - create sketch in the component containing
        // the surface
        LOG_DEBUG("Creating V-carve sketch in target surface component: '" << params.targetSurfaceId << "'");

        vcarveSketch = workspace_->createSketchInTargetComponent(vcarveSketchName, params.targetSurfaceId);
      } else if (!sourcePlaneId.empty()) {
        vcarveSketch = workspace_->createSketchOnPlane(vcarveSketchName, sourcePlaneId);
      } else if (!lastImportedPlaneEntityId_.empty()) {
        vcarveSketch = workspace_->createSketchOnPlane(vcarveSketchName, lastImportedPlaneEntityId_);
      } else {
        vcarveSketch = workspace_->createSketch(vcarveSketchName);
      }

      if (vcarveSketch) {
        // Generate V-carve toolpaths directly from medial axis results (in
        // memory)
        bool vcarveSuccess = generateVCarveToolpaths(allResults, params, vcarveSketch.get(), profileTransforms);
        if (vcarveSuccess) {
          vcarveSketch->finishSketch();
        }
      }
    }
    auto vcarveEnd = std::chrono::high_resolution_clock::now();
    auto vcarveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(vcarveEnd - vcarveStart);
    logger_->logInfo("⏱️ V-carve toolpath generation took: " + std::to_string(vcarveDuration.count()) + "ms");

    // Show results to user
    std::string resultMsg = "Medial Axis Generation Complete\n\n";
    resultMsg +=
        "Processed: " + std::to_string(successCount) + " of " + std::to_string(profilePolygons.size()) + " profiles\n";
    resultMsg += "Total Points: " + std::to_string(totalPoints) + "\n";
    resultMsg += "Total Length: " + std::to_string(static_cast<int>(totalLength)) + " mm\n\n";
    if (params.generateVisualization) {
      resultMsg += "Construction geometry created in sketch: " + sketchName;
    }

    if (params.generateVCarveToolpaths) {
      resultMsg += "\nV-carve toolpaths: " + ("V-Carve Toolpaths - " + params.toolName);
    }

    // Success popup removed - only log the results
    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart);
    logger_->logInfo("⏱️ === TOTAL GENERATE PATHS took: " + std::to_string(totalDuration.count()) + "ms ===");
    return true;
  } catch (const std::exception& e) {
    std::string errorMsg = "Failed to generate medial axis: " + std::string(e.what());
    ui_->showMessageBox("Medial Axis Generation - Error", errorMsg);
    return false;
  } catch (...) {
    std::string errorMsg = "Unknown error during medial axis generation";
    ui_->showMessageBox("Medial Axis Generation - Error", errorMsg);
    return false;
  }
}

}  // namespace Core
}  // namespace ChipCarving

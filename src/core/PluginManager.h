/**
 * Core plugin management logic
 * Separated from Fusion API for testability
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "geometry/MedialAxisProcessor.h"
#include "geometry/Shape.h"
#include "parsers/DesignParser.h"
#include "adapters/IFusionInterface.h"

namespace ChipCarving {
namespace Core {

/**
 * Main plugin business logic coordinator
 * Handles command execution without direct Fusion API dependencies
 */
class PluginManager {
  ///
  /// Setup error handler UI integration
  void setupErrorHandling();

  ///
  /// Setup error handler UI integration
 public:
  explicit PluginManager(std::unique_ptr<Adapters::IFusionFactory> factory);
  ~PluginManager() = default;

  // Plugin lifecycle
  bool initialize();
  void shutdown();

  // Command implementations
  bool executeImportDesign();
  bool executeImportDesign(const std::string& filePath, const std::string& planeEntityId = "");
  bool executeGeneratePaths();

  // Enhanced UI Phase 5: Medial axis generation with construction geometry
  bool executeMedialAxisGeneration(const Adapters::SketchSelection& selection,
                                   const Adapters::MedialAxisParameters& params);

  // Configuration methods
  void setMedialAxisParameters(double polygonTolerance, double medialThreshold);

  // Status and information
  std::string getVersion() const;
  std::string getName() const;
  bool isInitialized() const {
    return initialized_;
  }
  bool hasImportedShapes() const {
    return !importedShapes_.empty();
  }

  // Access to factory for UI operations
  Adapters::IFusionFactory* getFactory() const {
    return factory_.get();
  }

 private:
  std::unique_ptr<Adapters::IFusionFactory> factory_;
  std::unique_ptr<Adapters::ILogger> logger_;
  std::unique_ptr<Adapters::IUserInterface> ui_;
  std::unique_ptr<Adapters::IWorkspace> workspace_;

  // Imported design data
  std::vector<std::unique_ptr<Geometry::Shape>> importedShapes_;
  std::string lastImportedFile_;
  std::string lastImportedPlaneEntityId_;  // Store plane entity ID for medial
                                           // axis generation

  // Medial axis processing
  std::unique_ptr<Geometry::MedialAxisProcessor> medialProcessor_;

  bool initialized_;

  // Helper methods
  void addConstructionGeometryVisualization(Adapters::ISketch* sketch, const Geometry::MedialAxisResults& results,
                                            const Adapters::MedialAxisParameters& params,
                                            const Adapters::IWorkspace::TransformParams& transform,
                                            const std::vector<Geometry::Point2D>& polygon);

  // Enhanced UI Phase 5.2: Profile geometry extraction
  // Structure to hold profile geometry with transformation parameters
  struct ProfileData {
    std::vector<Geometry::Point2D> polygon;
    Adapters::IWorkspace::TransformParams transform;
  };

  bool extractProfileGeometry(const Adapters::SketchSelection& selection,
                              std::vector<std::vector<Geometry::Point2D>>& profilePolygons,
                              std::vector<Adapters::IWorkspace::TransformParams>& profileTransforms);

  void logStartup();
  void logShutdown();
  std::string formatMedialAxisResults(const Geometry::MedialAxisResults& results);

  /**
   * Generate V-carve toolpaths from medial axis results and add to sketch
   * @param medialResults Vector of medial axis computation results
   * @param params Parameters including V-carve settings
   * @param sketch Target sketch for V-carve toolpaths
   * @param transforms Coordinate transformations for each profile
   * @return true if V-carve generation succeeded
   */
  bool generateVCarveToolpaths(const std::vector<Geometry::MedialAxisResults>& medialResults,
                               const Adapters::MedialAxisParameters& params, Adapters::ISketch* sketch,
                               const std::vector<Adapters::IWorkspace::TransformParams>& transforms);
};

}  // namespace Core
}  // namespace ChipCarving

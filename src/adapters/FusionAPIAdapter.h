/**
 * Concrete implementation of Fusion 360 API interfaces
 * Wraps actual Fusion API calls for dependency injection
 */

#pragma once

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <fstream>
#include <memory>

#include "IFusionInterface.h"

namespace ChipCarving {
namespace Adapters {

/**
 * Fusion 360 logger implementation
 * Writes to file and Fusion's Text Commands palette
 */
class FusionLogger : public ILogger {
 public:
  explicit FusionLogger(const std::string& logFilePath);
  ~FusionLogger() override;

  void logInfo(const std::string& message) const override;
  void logDebug(const std::string& message) const override;
  void logWarning(const std::string& message) const override;
  void logError(const std::string& message) const override;

 private:
  mutable std::ofstream logFile_{};
  std::string logFilePath_{};

  void writeLog(const std::string& message, const std::string& level) const;
  void rotateLogFile();
  void checkAndRotateIfNeeded();
};

/**
 * Fusion 360 user interface implementation
 * Wraps Fusion UI operations
 */
class FusionUserInterface : public IUserInterface {
 public:
  explicit FusionUserInterface(adsk::core::Ptr<adsk::core::UserInterface> ui);
  ~FusionUserInterface() override = default;

  void showMessageBox(const std::string& title, const std::string& message) override;
  std::string showFileDialog(const std::string& title, const std::string& filter) override;
  std::string selectJsonFile() override;
  bool confirmAction(const std::string& message) override;

  // Enhanced UI operations
  bool showParameterDialog(const std::string& title, MedialAxisParameters& params) override;
  SketchSelection showSketchSelectionDialog(const std::string& title) override;
  void updateSelectionCount(int count) override;

 private:
  adsk::core::Ptr<adsk::core::UserInterface> ui_{};
};

/**
 * Fusion 360 sketch implementation
 * Wraps Fusion sketch operations
 */
class FusionSketch : public ISketch {
 public:
  explicit FusionSketch(const std::string& name, adsk::core::Ptr<adsk::core::Application> app,
                        adsk::core::Ptr<adsk::fusion::Sketch> sketch);
  ~FusionSketch() override = default;

  void addShape(const Geometry::Shape* shape, ILogger* logger = nullptr) override;
  std::string getName() const override;
  bool addLineToSketch(double x1, double y1, double x2, double y2) override;
  bool addArcToSketch(double centerX, double centerY, double radius, double startAngle, double endAngle) override;
  int addPointToSketch(double x, double y) override;
  bool addArcByThreePointsToSketch(int startPointIndex, int midPointIndex, int endPointIndex) override;
  bool addLineByTwoPointsToSketch(int startPointIndex, int endPointIndex) override;
  bool deleteSketchPoint(int pointIndex) override;
  void finishSketch() override;

  // Construction geometry methods
  bool addConstructionLine(double x1, double y1, double x2, double y2) override;
  bool addConstructionCircle(double centerX, double centerY, double radius) override;
  bool addConstructionPoint(double x, double y) override;
  void clearConstructionGeometry() override;

  // 3D sketch methods for V-carve toolpaths
  bool addSpline3D(const std::vector<Geometry::Point3D>& points) override;
  bool addLine3D(double x1, double y1, double z1, double x2, double y2, double z2) override;
  bool addPoint3D(double x, double y, double z) override;

  // Get 3D toolpath curves for solid operations
  std::vector<std::string> getSketchCurveEntityIds() override;

 private:
  std::string name_{};
  adsk::core::Ptr<adsk::core::Application> app_{};
  adsk::core::Ptr<adsk::fusion::Sketch> sketch_{};
  std::vector<adsk::core::Ptr<adsk::fusion::SketchPoint>> sketchPoints_{};

  // Construction geometry tracking
  std::vector<adsk::core::Ptr<adsk::fusion::SketchLine>> constructionLines_{};
  std::vector<adsk::core::Ptr<adsk::fusion::SketchCircle>> constructionCircles_{};
  std::vector<adsk::core::Ptr<adsk::fusion::SketchPoint>> constructionPoints_{};
};

/**
 * Fusion 360 workspace implementation
 * Wraps Fusion workspace and sketch operations
 */
class FusionWorkspace : public IWorkspace {
 public:
  explicit FusionWorkspace(adsk::core::Ptr<adsk::core::Application> app);
  ~FusionWorkspace() override = default;

  std::unique_ptr<ISketch> createSketch(const std::string& name) override;
  std::unique_ptr<ISketch> createSketchOnPlane(const std::string& name, const std::string& planeEntityId) override;
  std::unique_ptr<ISketch> createSketchInTargetComponent(const std::string& name,
                                                         const std::string& surfaceEntityId) override;
  std::unique_ptr<ISketch> findSketch(const std::string& name) override;
  std::vector<std::string> getAllSketchNames() override;

  // Enhanced UI Phase 5.2: Profile geometry extraction
  bool extractProfileVertices(const std::string& entityId, std::vector<std::pair<double, double>>& vertices,
                              TransformParams& transform) override;

  // Extract plane entity ID from a profile's parent sketch
  std::string extractPlaneEntityIdFromProfile(const std::string& profileEntityId) override;

  // Extract ProfileGeometry directly from a profile object
  bool extractProfileGeometry(adsk::core::Ptr<adsk::fusion::Profile> profile, ProfileGeometry& geometry);

  // Surface query methods for projection
  double getSurfaceZAtXY(const std::string& surfaceId, double x, double y) override;

 private:
  adsk::core::Ptr<adsk::core::Application> app_{};

  // Helper method for getting world geometry of sketch curves
  adsk::core::Ptr<adsk::core::Curve3D> getCurveWorldGeometry(adsk::core::Ptr<adsk::fusion::SketchCurve> sketchCurve);

  // Profile search operations (split for maintainability)
  adsk::core::Ptr<adsk::fusion::Profile> findProfileByEntityToken(const std::string& entityId);

  // Curve extraction operations (split for maintainability)
  bool extractCurvesFromProfile(adsk::core::Ptr<adsk::fusion::Profile> profile,
                                std::vector<struct CurveData>& allCurves, TransformParams& transform);

  // ============================================================================
  // Direct entity lookup using Design.findEntityByToken()
  // Added as part of API usage improvements - replaces manual iteration
  // See commit message for details on this change
  // ============================================================================

  // Find any entity by its token using the official Fusion API method
  // Returns empty vector if not found
  std::vector<adsk::core::Ptr<adsk::core::Base>> findEntitiesByToken(const std::string& entityToken);

  // Get the parent component from various entity types (BRepFace, BRepBody, Profile, etc.)
  // Returns nullptr if component cannot be determined
  adsk::core::Ptr<adsk::fusion::Component> getComponentFromEntity(adsk::core::Ptr<adsk::core::Base> entity);

  // Log detailed error information from Fusion API using getLastError()
  // Call this after an API operation returns null/false for better diagnostics
  void logApiError(const std::string& operation) const;
};

/**
 * Factory for creating Fusion 360 API implementations
 * Provides dependency injection for real Fusion operations
 */
class FusionAPIFactory : public IFusionFactory {
 public:
  FusionAPIFactory(adsk::core::Ptr<adsk::core::Application> app, adsk::core::Ptr<adsk::core::UserInterface> ui,
                   const std::string& logFilePath);
  ~FusionAPIFactory() override = default;

  std::unique_ptr<ILogger> createLogger() override;
  std::unique_ptr<IUserInterface> createUserInterface() override;
  std::unique_ptr<IWorkspace> createWorkspace() override;

 private:
  adsk::core::Ptr<adsk::core::Application> app_{};
  adsk::core::Ptr<adsk::core::UserInterface> ui_{};
  std::string logFilePath_{};
};

}  // namespace Adapters
}  // namespace ChipCarving

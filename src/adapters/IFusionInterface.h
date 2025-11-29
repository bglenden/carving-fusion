/**
 * Abstract interface for Fusion 360 API dependencies
 * Enables dependency injection and testing with mocks
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace ChipCarving {
namespace Geometry {
class Shape;
struct Point2D;
struct Point3D;
}  // namespace Geometry
}  // namespace ChipCarving

namespace ChipCarving {
namespace Adapters {

/**
 * Abstract interface for logging operations
 * Allows testing without actual file I/O or Fusion UI
 */
class ILogger {
 public:
  virtual ~ILogger() = default;

  virtual void logInfo(const std::string& message) const = 0;
  virtual void logDebug(const std::string& message) const = 0;
  virtual void logWarning(const std::string& message) const = 0;
  virtual void logError(const std::string& message) const = 0;
};

/**
 * Structure for medial axis processing parameters
 */
struct MedialAxisParameters {
  double polygonTolerance = 0.25;          // Maximum polygon approximation error (mm)
  double samplingDistance = 1.0;           // Distance between sampled points (mm)
  double clearanceCircleSpacing = 5.0;     // Distance between clearance circles (mm)
  double crossSize = 3.0;                  // Size of center cross marks in mm (0 = no crosses)
  bool forceBoundaryIntersections = true;  // Force sampling at boundary intersections
  bool showMedialLines = true;             // Show medial axis lines in construction geometry
  bool showClearanceCircles = true;        // Show clearance circles in construction geometry
  bool showPolygonizedShape = false;       // Show polygonized shape outline
  bool generateVisualization = false;      // Generate visualization sketches (default off)

  // Tool parameters for V-carve generation
  std::string toolName = "90° V-bit";  // Tool name for sketch naming
  double toolAngle = 90.0;             // V-bit angle in degrees
  double toolDiameter = 6.35;          // Tool diameter in mm (1/4 inch default)

  // V-carve toolpath parameters
  bool generateVCarveToolpaths = false;  // Generate V-carve toolpaths (default off)
  double maxVCarveDepth = 25.0;          // Maximum V-carve depth in mm (safety limit, default 25mm)

  // Surface projection parameters
  std::string targetSurfaceId{};  // Entity ID of surface to project onto (empty =
                                  // XY plane)
  bool projectToSurface = true;   // Always project toolpaths onto surface
};

// Forward declaration for ProfileGeometry
struct ProfileGeometry;

/**
 * Structure for sketch selection results
 */
struct SketchSelection {
  std::vector<std::string> selectedEntityIds{};   // Fusion entity IDs of selected paths (DEPRECATED -
                                                  // use selectedProfiles)
  std::vector<ProfileGeometry> selectedProfiles{};  // NEW: Extracted profile geometry
  int closedPathCount = 0;                        // Number of valid closed paths
  bool isValid = false;                           // Whether selection is valid for processing
  std::string errorMessage{};                     // Error message if invalid
};

/**
 * Abstract interface for user interface operations
 * Allows testing without Fusion 360 UI dependencies
 */
class IUserInterface {
 public:
  virtual ~IUserInterface() = default;

  // Basic UI operations
  virtual void showMessageBox(const std::string& title, const std::string& message) = 0;
  virtual std::string showFileDialog(const std::string& title, const std::string& filter) = 0;
  virtual std::string selectJsonFile() = 0;
  virtual bool confirmAction(const std::string& message) = 0;

  // Enhanced UI operations for Generate Paths command
  virtual bool showParameterDialog(const std::string& title, MedialAxisParameters& params) = 0;
  virtual SketchSelection showSketchSelectionDialog(const std::string& title) = 0;
  virtual void updateSelectionCount(int count) = 0;
};

/**
 * Abstract interface for Fusion 360 sketch operations
 * Allows testing without actual Fusion 360 sketch API
 */
class ISketch {
 public:
  virtual ~ISketch() = default;

  virtual void addShape(const Geometry::Shape* shape, ILogger* logger = nullptr) = 0;
  virtual std::string getName() const = 0;
  virtual bool addLineToSketch(double x1, double y1, double x2, double y2) = 0;
  virtual bool addArcToSketch(double centerX, double centerY, double radius, double startAngle, double endAngle) = 0;
  virtual int addPointToSketch(double x, double y) = 0;
  virtual bool addArcByThreePointsToSketch(int startPointIndex, int midPointIndex, int endPointIndex) = 0;
  virtual bool addLineByTwoPointsToSketch(int startPointIndex, int endPointIndex) = 0;
  virtual bool deleteSketchPoint(int pointIndex) = 0;
  virtual void finishSketch() = 0;

  // Construction geometry methods
  virtual bool addConstructionLine(double x1, double y1, double x2, double y2) = 0;
  virtual bool addConstructionCircle(double centerX, double centerY, double radius) = 0;
  virtual bool addConstructionPoint(double x, double y) = 0;
  virtual void clearConstructionGeometry() = 0;

  // 3D sketch methods for V-carve toolpaths
  virtual bool addSpline3D(const std::vector<Geometry::Point3D>& points) = 0;
  virtual bool addLine3D(double x1, double y1, double z1, double x2, double y2, double z2) = 0;
  virtual bool addPoint3D(double x, double y, double z) = 0;

  // Get 3D toolpath curves for solid operations
  virtual std::vector<std::string> getSketchCurveEntityIds() = 0;
};

/**
 * Abstract interface for Fusion 360 workspace operations
 * Allows testing without actual Fusion 360 workspace
 */
class IWorkspace {
 public:
  virtual ~IWorkspace() = default;

  virtual std::unique_ptr<ISketch> createSketch(const std::string& name) = 0;

  // Create sketch on specific plane/surface
  virtual std::unique_ptr<ISketch> createSketchOnPlane(const std::string& name, const std::string& planeEntityId) = 0;

  // Create sketch in component that contains the target surface (enhanced for
  // cross-component organization)
  virtual std::unique_ptr<ISketch> createSketchInTargetComponent(const std::string& name,
                                                                 const std::string& surfaceEntityId) = 0;

  // Find existing sketch by name
  virtual std::unique_ptr<ISketch> findSketch(const std::string& name) = 0;

  // Get all sketch names in the workspace
  virtual std::vector<std::string> getAllSketchNames() = 0;

  // Enhanced UI Phase 5.2: Profile geometry extraction
  // Structure to hold transformation parameters from world to unit circle
  // coordinates
  struct TransformParams {
    double centerX = 0.0;       // World space center X (cm)
    double centerY = 0.0;       // World space center Y (cm)
    double scale = 1.0;         // Scale factor from world to unit circle
    double sketchPlaneZ = 0.0;  // Z position of the sketch plane (cm)
  };

  virtual bool extractProfileVertices(const std::string& entityId, std::vector<std::pair<double, double>>& vertices,
                                      TransformParams& transform) = 0;

  // Extract plane entity ID from a profile's parent sketch
  // Returns the entity ID of the plane/surface the profile's sketch is created
  // on Returns empty string if extraction fails
  virtual std::string extractPlaneEntityIdFromProfile(const std::string& profileEntityId) = 0;

  // Surface query methods for projection
  // Get the Z height of a surface at a specific XY location
  // Returns the Z coordinate where a vertical line through (x,y) intersects the
  // surface Returns NaN if no intersection found
  virtual double getSurfaceZAtXY(const std::string& surfaceId, double x, double y) = 0;
};

/**
 * Structure to store extracted profile geometry
 */
struct ProfileGeometry {
  std::vector<std::pair<double, double>> vertices{};  // Profile vertices in world coordinates (cm)
  IWorkspace::TransformParams transform{};            // Transform parameters for the profile
  std::string sketchName{};                           // Parent sketch name for debugging
  double area = 0.0;                                  // Area from areaProperties (sq cm)
  std::pair<double, double> centroid{0.0, 0.0};       // Centroid from areaProperties (cm)
  std::string planeEntityId{};                        // Entity ID of the sketch plane
};

/**
 * Factory interface for creating Fusion-dependent objects
 * Enables dependency injection throughout the plugin
 */
class IFusionFactory {
 public:
  virtual ~IFusionFactory() = default;

  virtual std::unique_ptr<ILogger> createLogger() = 0;
  virtual std::unique_ptr<IUserInterface> createUserInterface() = 0;
  virtual std::unique_ptr<IWorkspace> createWorkspace() = 0;
};

}  // namespace Adapters
}  // namespace ChipCarving

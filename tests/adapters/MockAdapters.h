/**
 * Mock implementations for testing
 * Allows unit testing without Fusion 360 dependencies
 */

#pragma once

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "../../include/geometry/Shape.h"
#include "../../include/geometry/Point3D.h"
#include "../../src/adapters/IFusionInterface.h"

using namespace ChipCarving::Adapters;

/**
 * Mock logger for testing
 * Captures log messages for verification
 */
class MockLogger : public ILogger {
   public:
    void logInfo(const std::string& message) const override {
        infoMessages.push_back(message);
    }

    void logDebug(const std::string& message) const override {
        debugMessages.push_back(message);
    }

    void logWarning(const std::string& message) const override {
        warningMessages.push_back(message);
    }

    void logError(const std::string& message) const override {
        errorMessages.push_back(message);
    }

    // Test helpers (mutable to allow const methods to modify)
    mutable std::vector<std::string> infoMessages;
    mutable std::vector<std::string> debugMessages;
    mutable std::vector<std::string> warningMessages;
    mutable std::vector<std::string> errorMessages;

    void clearMessages() {
        infoMessages.clear();
        debugMessages.clear();
        warningMessages.clear();
        errorMessages.clear();
    }
};

/**
 * Mock user interface for testing
 * Captures UI interactions for verification
 */
class MockUserInterface : public IUserInterface {
   public:
    void showMessageBox(const std::string& title, const std::string& message) override {
        lastMessageBoxTitle = title;
        lastMessageBoxMessage = message;
        messageBoxCallCount++;
    }

    std::string showFileDialog(const std::string& title, const std::string& filter) override {
        lastFileDialogTitle = title;
        lastFileDialogFilter = filter;
        return mockFileDialogPath;
    }

    std::string selectJsonFile() override {
        return mockJsonFilePath;
    }

    bool confirmAction(const std::string& message) override {
        lastConfirmMessage = message;
        return mockConfirmResult;
    }

    // Enhanced UI operations for Generate Paths command
    bool showParameterDialog(const std::string& title, MedialAxisParameters& params) override {
        lastParameterDialogTitle = title;
        lastParameterDialogParams = params;
        parameterDialogCallCount++;

        // Apply mock values if needed
        if (mockParameterDialogResult) {
            params = mockParameterValues;
        }

        return mockParameterDialogResult;
    }

    SketchSelection showSketchSelectionDialog(const std::string& title) override {
        lastSketchSelectionDialogTitle = title;
        sketchSelectionDialogCallCount++;
        return mockSketchSelection;
    }

    void updateSelectionCount(int count) override {
        lastSelectionCount = count;
        updateSelectionCountCallCount++;
    }

    // Test helpers
    std::string lastMessageBoxTitle;
    std::string lastMessageBoxMessage;
    int messageBoxCallCount = 0;

    std::string lastFileDialogTitle;
    std::string lastFileDialogFilter;
    std::string mockFileDialogPath = "/test/path/design.json";

    std::string lastConfirmMessage;
    bool mockConfirmResult = true;

    std::string mockJsonFilePath = "/test/path/design.json";

    // Enhanced UI test helpers
    std::string lastParameterDialogTitle;
    MedialAxisParameters lastParameterDialogParams;
    int parameterDialogCallCount = 0;
    bool mockParameterDialogResult = true;
    MedialAxisParameters mockParameterValues;

    std::string lastSketchSelectionDialogTitle;
    int sketchSelectionDialogCallCount = 0;
    SketchSelection mockSketchSelection;

    int lastSelectionCount = 0;
    int updateSelectionCountCallCount = 0;

    void reset() {
        lastMessageBoxTitle.clear();
        lastMessageBoxMessage.clear();
        messageBoxCallCount = 0;
        lastFileDialogTitle.clear();
        lastFileDialogFilter.clear();
        mockFileDialogPath = "/test/path/design.json";
        lastConfirmMessage.clear();
        mockConfirmResult = true;
        mockJsonFilePath = "/test/path/design.json";

        // Reset enhanced UI state
        lastParameterDialogTitle.clear();
        parameterDialogCallCount = 0;
        mockParameterDialogResult = true;
        mockParameterValues = MedialAxisParameters{};
        lastSketchSelectionDialogTitle.clear();
        sketchSelectionDialogCallCount = 0;
        mockSketchSelection = SketchSelection{};
        lastSelectionCount = 0;
        updateSelectionCountCallCount = 0;
    }
};

/**
 * Mock sketch for testing
 * Captures sketch operations for verification
 */
class MockSketch : public ISketch {
   public:
    MockSketch(const std::string& name) : name_(name) {}

    void addShape(const ChipCarving::Geometry::Shape* shape, ILogger* logger = nullptr) override {
        addedShapes.push_back(shape);
        addShapeCallCount++;

        // Delegate to the shape to draw itself
        if (shape) {
            shape->drawToSketch(this, logger);
        }
    }

    std::string getName() const override {
        return name_;
    }

    bool addLineToSketch(double x1, double y1, double x2, double y2) override {
        lines.push_back({x1, y1, x2, y2});
        return mockAddLineResult;
    }

    bool addArcToSketch(double centerX, double centerY, double radius, double startAngle,
                        double endAngle) override {
        arcs.push_back({centerX, centerY, radius, startAngle, endAngle});
        return mockAddArcResult;
    }

    int addPointToSketch(double x, double y) override {
        if (mockAddPointResult) {
            points.push_back({x, y});
            return points.size() - 1;  // Return the index of the added point
        } else {
            return -1;  // Return -1 for failure
        }
    }

    bool addArcByThreePointsToSketch(int startPointIndex, int midPointIndex,
                                     int endPointIndex) override {
        threePointArcs.push_back({startPointIndex, midPointIndex, endPointIndex});
        return mockAddThreePointArcResult;
    }

    bool addLineByTwoPointsToSketch(int startPointIndex, int endPointIndex) override {
        twoPointLines.push_back({startPointIndex, endPointIndex});
        return mockAddTwoPointLineResult;
    }

    bool deleteSketchPoint(int pointIndex) override {
        if (pointIndex >= 0 && pointIndex < static_cast<int>(points.size())) {
            deletedPointIndices.push_back(pointIndex);
            return mockDeletePointResult;
        }
        return false;
    }

    void finishSketch() override {
        finishSketchCallCount++;
    }

    // Construction geometry methods
    bool addConstructionLine(double x1, double y1, double x2, double y2) override {
        constructionLines.push_back({x1, y1, x2, y2});
        return mockAddConstructionLineResult;
    }

    bool addConstructionCircle(double centerX, double centerY, double radius) override {
        constructionCircles.push_back({centerX, centerY, radius});
        return mockAddConstructionCircleResult;
    }

    bool addConstructionPoint(double x, double y) override {
        constructionPoints.push_back({x, y});
        return mockAddConstructionPointResult;
    }

    void clearConstructionGeometry() override {
        constructionLines.clear();
        constructionCircles.clear();
        constructionPoints.clear();
        clearConstructionGeometryCallCount++;
    }
    
    // 3D sketch methods for V-carve toolpaths
    bool addSpline3D(const std::vector<ChipCarving::Geometry::Point3D>& points) override {
        if (points.size() < 2) {
            return false;
        }
        splines3D.push_back(points);
        return true;
    }
    
    bool addLine3D(double x1, double y1, double z1, double x2, double y2, double z2) override {
        lines3D.push_back({ChipCarving::Geometry::Point3D(x1, y1, z1), ChipCarving::Geometry::Point3D(x2, y2, z2)});
        return true;
    }
    
    bool addPoint3D(double x, double y, double z) override {
        points3D.push_back(ChipCarving::Geometry::Point3D(x, y, z));
        return true;
    }
    
    std::vector<std::string> getSketchCurveEntityIds() override {
        return mockCurveEntityIds;
    }

    // Test helpers
    struct Line {
        double x1, y1, x2, y2;
    };
    struct Arc {
        double centerX, centerY, radius, startAngle, endAngle;
    };
    struct Point {
        double x, y;
    };
    struct Circle {
        double centerX, centerY, radius;
    };
    struct ThreePointArc {
        int startIdx, midIdx, endIdx;
    };
    struct TwoPointLine {
        int startIdx, endIdx;
    };

    std::string name_;
    std::vector<const ChipCarving::Geometry::Shape*> addedShapes;
    int addShapeCallCount = 0;
    std::vector<Line> lines;
    std::vector<Arc> arcs;
    std::vector<Point> points;
    std::vector<ThreePointArc> threePointArcs;
    std::vector<TwoPointLine> twoPointLines;
    std::vector<int> deletedPointIndices;
    int finishSketchCallCount = 0;

    // Construction geometry test helpers
    std::vector<Line> constructionLines;
    std::vector<Circle> constructionCircles;
    std::vector<Point> constructionPoints;
    int clearConstructionGeometryCallCount = 0;
    
    // 3D geometry collections
    std::vector<std::vector<ChipCarving::Geometry::Point3D>> splines3D;
    std::vector<std::pair<ChipCarving::Geometry::Point3D, ChipCarving::Geometry::Point3D>> lines3D;
    std::vector<ChipCarving::Geometry::Point3D> points3D;
    
    // Mock entity IDs for sketch curves
    std::vector<std::string> mockCurveEntityIds;

    bool mockAddLineResult = true;
    bool mockAddArcResult = true;
    bool mockAddPointResult = true;
    bool mockAddThreePointArcResult = true;
    bool mockAddTwoPointLineResult = true;
    bool mockDeletePointResult = true;
    bool mockAddConstructionLineResult = true;
    bool mockAddConstructionCircleResult = true;
    bool mockAddConstructionPointResult = true;

    void reset() {
        addedShapes.clear();
        addShapeCallCount = 0;
        lines.clear();
        arcs.clear();
        points.clear();
        threePointArcs.clear();
        twoPointLines.clear();
        deletedPointIndices.clear();
        finishSketchCallCount = 0;

        // Reset construction geometry
        constructionLines.clear();
        constructionCircles.clear();
        constructionPoints.clear();
        clearConstructionGeometryCallCount = 0;
        
        // Reset 3D geometry and mock entity IDs
        splines3D.clear();
        lines3D.clear();
        points3D.clear();
        mockCurveEntityIds.clear();

        mockAddLineResult = true;
        mockAddArcResult = true;
        mockAddPointResult = true;
        mockAddThreePointArcResult = true;
        mockAddTwoPointLineResult = true;
        mockDeletePointResult = true;
        mockAddConstructionLineResult = true;
        mockAddConstructionCircleResult = true;
        mockAddConstructionPointResult = true;
    }
};

/**
 * Mock workspace for testing
 * Captures workspace operations for verification
 */
class MockWorkspace : public IWorkspace {
   public:
    std::unique_ptr<ISketch> createSketch(const std::string& name) override {
        lastSketchName = name;
        createSketchCallCount++;

        if (mockCreateSketchResult) {
            auto sketch = std::make_unique<MockSketch>(name);
            lastCreatedSketch = sketch.get();  // Store pointer for testing
            return std::move(sketch);
        }
        return nullptr;
    }
    
    std::unique_ptr<ISketch> createSketchOnPlane(const std::string& name, 
                                                  const std::string& planeEntityId) override {
        lastSketchName = name;
        lastPlaneEntityId = planeEntityId;
        createSketchOnPlaneCallCount++;

        if (mockCreateSketchOnPlaneResult) {
            auto sketch = std::make_unique<MockSketch>(name);
            lastCreatedSketch = sketch.get();  // Store pointer for testing
            return std::move(sketch);
        }
        return nullptr;
    }
    
    std::unique_ptr<ISketch> createSketchInTargetComponent(const std::string& name,
                                                           const std::string& surfaceEntityId) override {
        lastSketchName = name;
        lastTargetSurfaceEntityId = surfaceEntityId;
        createSketchInTargetComponentCallCount++;

        if (mockCreateSketchInTargetComponentResult) {
            auto sketch = std::make_unique<MockSketch>(name);
            lastCreatedSketch = sketch.get();  // Store pointer for testing
            return std::move(sketch);
        }
        return nullptr;
    }
    
    std::unique_ptr<ISketch> findSketch(const std::string& name) override {
        lastFindSketchName = name;
        findSketchCallCount++;
        
        if (mockFindSketchResult) {
            // Return a new mock sketch with the requested name
            auto sketch = std::make_unique<MockSketch>(name);
            lastFoundSketch = sketch.get();  // Store pointer for testing
            return std::move(sketch);
        }
        return nullptr;
    }

    // Enhanced UI Phase 5.2: Profile geometry extraction
    bool extractProfileVertices(const std::string& entityId,
                                std::vector<std::pair<double, double>>& vertices,
                                TransformParams& transform) override {
        lastExtractedEntityId = entityId;
        extractProfileVerticesCallCount++;

        if (mockExtractProfileVerticesResult) {
            vertices = mockProfileVertices;
            // Set default transform for tests
            transform.centerX = 0.0;
            transform.centerY = 0.0;
            transform.scale = 1.0;
            transform.sketchPlaneZ = mockSketchPlaneZ;
            return true;
        }

        return false;
    }
    
    // Extract plane entity ID from a profile's parent sketch
    std::string extractPlaneEntityIdFromProfile(const std::string& profileEntityId) override {
        lastExtractedPlaneProfileId = profileEntityId;
        extractPlaneCallCount++;
        return mockPlaneEntityId;
    }
    
    // Surface query methods for projection
    double getSurfaceZAtXY(const std::string& surfaceId, double x, double y) override {
        lastQueriedSurfaceId = surfaceId;
        lastQueriedX = x;
        lastQueriedY = y;
        getSurfaceZCallCount++;
        
        // Return mock surface Z or NaN if no surface
        if (mockSurfaceZResult) {
            return mockSurfaceZ;
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Solid modeling operations for carving simulation
    std::string createVBitSolid(double toolAngle, double toolDiameter, double height) {
        lastVBitToolAngle = toolAngle;
        lastVBitToolDiameter = toolDiameter;
        lastVBitHeight = height;
        createVBitSolidCallCount++;
        return mockVBitSolidId;
    }
    
    bool sweepSolidAlongPath(const std::string& toolSolidId, const std::string& pathEntityId) {
        lastSweepToolSolidId = toolSolidId;
        lastSweepPathEntityId = pathEntityId;
        sweepSolidAlongPathCallCount++;
        return mockSweepResult;
    }
    
    bool performBooleanCut(const std::string& targetBodyId, const std::string& toolBodyId) {
        lastBooleanTargetBodyId = targetBodyId;
        lastBooleanToolBodyId = toolBodyId;
        performBooleanCutCallCount++;
        return mockBooleanCutResult;
    }
    
    std::vector<std::string> getAllBodyIds() {
        getAllBodyIdsCallCount++;
        return mockBodyIds;
    }
    
    std::string createBoxSolid(double width, double height, double depth) {
        lastBoxWidth = width;
        lastBoxHeight = height;
        lastBoxDepth = depth;
        createBoxSolidCallCount++;
        return mockBoxSolidId;
    }
    
    std::vector<std::string> getAllSketchNames() override {
        getAllSketchNamesCallCount++;
        return mockSketchNames;
    }

    // Test helpers
    std::string lastSketchName;
    int createSketchCallCount = 0;
    MockSketch* lastCreatedSketch = nullptr;

    bool mockCreateSketchResult = true;
    
    // createSketchOnPlane test helpers
    std::string lastPlaneEntityId;
    int createSketchOnPlaneCallCount = 0;
    bool mockCreateSketchOnPlaneResult = true;
    
    // createSketchInTargetComponent test helpers
    std::string lastTargetSurfaceEntityId;
    int createSketchInTargetComponentCallCount = 0;
    bool mockCreateSketchInTargetComponentResult = true;
    
    // findSketch test helpers
    std::string lastFindSketchName;
    int findSketchCallCount = 0;
    MockSketch* lastFoundSketch = nullptr;
    bool mockFindSketchResult = false;  // Default to not finding sketches

    // Enhanced UI Phase 5.2: Profile geometry extraction test helpers
    std::string lastExtractedEntityId;
    int extractProfileVerticesCallCount = 0;
    bool mockExtractProfileVerticesResult = true;
    std::vector<std::pair<double, double>> mockProfileVertices;
    double mockSketchPlaneZ = 0.0;  // Default sketch plane at Z=0
    
    // Extract plane entity ID test helpers
    std::string lastExtractedPlaneProfileId;
    int extractPlaneCallCount = 0;
    std::string mockPlaneEntityId = "";  // Default to empty (no plane)
    
    // Surface query test helpers
    std::string lastQueriedSurfaceId;
    double lastQueriedX = 0.0;
    double lastQueriedY = 0.0;
    int getSurfaceZCallCount = 0;
    bool mockSurfaceZResult = false;
    double mockSurfaceZ = 0.0;
    
    // Solid modeling test helpers
    double lastVBitToolAngle = 0.0;
    double lastVBitToolDiameter = 0.0;
    double lastVBitHeight = 0.0;
    int createVBitSolidCallCount = 0;
    std::string mockVBitSolidId = "mock_vbit_solid";
    
    std::string lastSweepToolSolidId;
    std::string lastSweepPathEntityId;
    int sweepSolidAlongPathCallCount = 0;
    bool mockSweepResult = true;
    
    std::string lastBooleanTargetBodyId;
    std::string lastBooleanToolBodyId;
    int performBooleanCutCallCount = 0;
    bool mockBooleanCutResult = true;
    
    int getAllBodyIdsCallCount = 0;
    std::vector<std::string> mockBodyIds;
    
    double lastBoxWidth = 0.0;
    double lastBoxHeight = 0.0;
    double lastBoxDepth = 0.0;
    int createBoxSolidCallCount = 0;
    std::string mockBoxSolidId = "mock_box_solid";
    
    int getAllSketchNamesCallCount = 0;
    std::vector<std::string> mockSketchNames = {"Imported Design", "V-Carve Toolpaths - 90° V-bit", "Test Sketch"};

    void reset() {
        lastSketchName.clear();
        createSketchCallCount = 0;
        lastCreatedSketch = nullptr;
        mockCreateSketchResult = true;
        
        // Reset createSketchOnPlane fields
        lastPlaneEntityId.clear();
        createSketchOnPlaneCallCount = 0;
        mockCreateSketchOnPlaneResult = true;
        
        // Reset createSketchInTargetComponent fields
        lastTargetSurfaceEntityId.clear();
        createSketchInTargetComponentCallCount = 0;
        mockCreateSketchInTargetComponentResult = true;
        
        // Reset findSketch fields
        lastFindSketchName.clear();
        findSketchCallCount = 0;
        lastFoundSketch = nullptr;
        mockFindSketchResult = false;

        // Reset Enhanced UI Phase 5.2 fields
        lastExtractedEntityId.clear();
        extractProfileVerticesCallCount = 0;
        mockExtractProfileVerticesResult = true;
        mockProfileVertices.clear();
        mockSketchPlaneZ = 0.0;
        
        // Reset extract plane entity ID fields
        lastExtractedPlaneProfileId.clear();
        extractPlaneCallCount = 0;
        mockPlaneEntityId.clear();
        
        // Reset surface query fields
        lastQueriedSurfaceId.clear();
        lastQueriedX = 0.0;
        lastQueriedY = 0.0;
        getSurfaceZCallCount = 0;
        mockSurfaceZResult = false;
        mockSurfaceZ = 0.0;
        
        // Reset solid modeling fields
        lastVBitToolAngle = 0.0;
        lastVBitToolDiameter = 0.0;
        lastVBitHeight = 0.0;
        createVBitSolidCallCount = 0;
        mockVBitSolidId = "mock_vbit_solid";
        
        lastSweepToolSolidId.clear();
        lastSweepPathEntityId.clear();
        sweepSolidAlongPathCallCount = 0;
        mockSweepResult = true;
        
        lastBooleanTargetBodyId.clear();
        lastBooleanToolBodyId.clear();
        performBooleanCutCallCount = 0;
        mockBooleanCutResult = true;
        
        getAllBodyIdsCallCount = 0;
        mockBodyIds.clear();
        
        lastBoxWidth = 0.0;
        lastBoxHeight = 0.0;
        lastBoxDepth = 0.0;
        createBoxSolidCallCount = 0;
        mockBoxSolidId = "mock_box_solid";
        
        getAllSketchNamesCallCount = 0;
        mockSketchNames = {"Imported Design", "V-Carve Toolpaths - 90° V-bit", "Test Sketch"};
    }
};

/**
 * Non-deleting deleter for shared mock objects
 */
template <typename T>
struct NoOpDeleter {
    void operator()(T*) const {
        // Do nothing - shared_ptr manages the lifetime
    }
};

/**
 * Mock factory for creating test objects
 * Provides dependency injection for testing
 */
class MockFactory : public IFusionFactory {
   public:
    MockFactory() {
        mockLogger_ = std::make_shared<MockLogger>();
        mockUI_ = std::make_shared<MockUserInterface>();
        mockWorkspace_ = std::make_shared<MockWorkspace>();
    }

    std::unique_ptr<ILogger> createLogger() override {
        // Create a new mock each time, but keep a reference for testing
        auto logger = std::make_unique<MockLogger>();
        lastCreatedLogger_ = logger.get();
        return logger;
    }

    std::unique_ptr<IUserInterface> createUserInterface() override {
        // Create a new mock each time, but keep a reference for testing
        auto ui = std::make_unique<MockUserInterface>();
        lastCreatedUI_ = ui.get();
        return ui;
    }

    std::unique_ptr<IWorkspace> createWorkspace() override {
        // Create a new mock each time, but keep a reference for testing
        auto workspace = std::make_unique<MockWorkspace>();
        lastCreatedWorkspace_ = workspace.get();
        return workspace;
    }

    // Access to mock objects for test verification
    MockLogger* getLastCreatedLogger() {
        return lastCreatedLogger_;
    }
    MockUserInterface* getLastCreatedUI() {
        return lastCreatedUI_;
    }
    MockWorkspace* getLastCreatedWorkspace() {
        return lastCreatedWorkspace_;
    }

    // Legacy accessors for compatibility
    std::shared_ptr<MockLogger> getMockLogger() {
        return mockLogger_;
    }
    std::shared_ptr<MockUserInterface> getMockUI() {
        return mockUI_;
    }
    std::shared_ptr<MockWorkspace> getMockWorkspace() {
        return mockWorkspace_;
    }

   private:
    std::shared_ptr<MockLogger> mockLogger_;
    std::shared_ptr<MockUserInterface> mockUI_;
    std::shared_ptr<MockWorkspace> mockWorkspace_;

    // Raw pointers to last created objects for test verification
    MockLogger* lastCreatedLogger_ = nullptr;
    MockUserInterface* lastCreatedUI_ = nullptr;
    MockWorkspace* lastCreatedWorkspace_ = nullptr;
};
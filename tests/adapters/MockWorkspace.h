/**
 * MockWorkspace.h
 * Mock workspace for testing - captures workspace operations for verification
 */

#pragma once

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "MockSketch.h"
#include "adapters/IFusionInterface.h"

using namespace ChipCarving::Adapters;

class MockWorkspace : public IWorkspace {
 public:
  std::unique_ptr<ISketch> createSketch(const std::string& name) override {
    lastSketchName = name;
    createSketchCallCount++;

    if (mockCreateSketchResult) {
      auto sketch = std::make_unique<MockSketch>(name);
      lastCreatedSketch = sketch.get();
      return sketch;
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
      lastCreatedSketch = sketch.get();
      return sketch;
    }
    return nullptr;
  }

  std::unique_ptr<ISketch> createSketchInTargetComponent(
      const std::string& name, const std::string& surfaceEntityId) override {
    lastSketchName = name;
    lastTargetSurfaceEntityId = surfaceEntityId;
    createSketchInTargetComponentCallCount++;

    if (mockCreateSketchInTargetComponentResult) {
      auto sketch = std::make_unique<MockSketch>(name);
      lastCreatedSketch = sketch.get();
      return sketch;
    }
    return nullptr;
  }

  std::unique_ptr<ISketch> findSketch(const std::string& name) override {
    lastFindSketchName = name;
    findSketchCallCount++;

    if (mockFindSketchResult) {
      auto sketch = std::make_unique<MockSketch>(name);
      lastFoundSketch = sketch.get();
      return sketch;
    }
    return nullptr;
  }

  bool extractProfileVertices(const std::string& entityId,
                              std::vector<std::pair<double, double>>& vertices,
                              TransformParams& transform) override {
    lastExtractedEntityId = entityId;
    extractProfileVerticesCallCount++;

    if (mockExtractProfileVerticesResult) {
      vertices = mockProfileVertices;
      transform.centerX = 0.0;
      transform.centerY = 0.0;
      transform.scale = 1.0;
      transform.sketchPlaneZ = mockSketchPlaneZ;
      return true;
    }

    return false;
  }

  std::string extractPlaneEntityIdFromProfile(const std::string& profileEntityId) override {
    lastExtractedPlaneProfileId = profileEntityId;
    extractPlaneCallCount++;
    return mockPlaneEntityId;
  }

  double getSurfaceZAtXY(const std::string& surfaceId, double x, double y) override {
    lastQueriedSurfaceId = surfaceId;
    lastQueriedX = x;
    lastQueriedY = y;
    getSurfaceZCallCount++;

    if (mockSurfaceZResult) {
      return mockSurfaceZ;
    }
    return std::numeric_limits<double>::quiet_NaN();
  }

  std::vector<std::string> getAllSketchNames() override {
    getAllSketchNamesCallCount++;
    return mockSketchNames;
  }

  // Additional solid modeling methods (not in interface but used by some tests)
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

  // Test state - createSketch
  std::string lastSketchName;
  int createSketchCallCount = 0;
  MockSketch* lastCreatedSketch = nullptr;
  bool mockCreateSketchResult = true;

  // createSketchOnPlane
  std::string lastPlaneEntityId;
  int createSketchOnPlaneCallCount = 0;
  bool mockCreateSketchOnPlaneResult = true;

  // createSketchInTargetComponent
  std::string lastTargetSurfaceEntityId;
  int createSketchInTargetComponentCallCount = 0;
  bool mockCreateSketchInTargetComponentResult = true;

  // findSketch
  std::string lastFindSketchName;
  int findSketchCallCount = 0;
  MockSketch* lastFoundSketch = nullptr;
  bool mockFindSketchResult = false;

  // extractProfileVertices
  std::string lastExtractedEntityId;
  int extractProfileVerticesCallCount = 0;
  bool mockExtractProfileVerticesResult = true;
  std::vector<std::pair<double, double>> mockProfileVertices;
  double mockSketchPlaneZ = 0.0;

  // extractPlaneEntityIdFromProfile
  std::string lastExtractedPlaneProfileId;
  int extractPlaneCallCount = 0;
  std::string mockPlaneEntityId;

  // getSurfaceZAtXY
  std::string lastQueriedSurfaceId;
  double lastQueriedX = 0.0;
  double lastQueriedY = 0.0;
  int getSurfaceZCallCount = 0;
  bool mockSurfaceZResult = false;
  double mockSurfaceZ = 0.0;

  // getAllSketchNames
  int getAllSketchNamesCallCount = 0;
  std::vector<std::string> mockSketchNames = {"Imported Design", "V-Carve Toolpaths - 90° V-bit",
                                               "Test Sketch"};

  // Solid modeling
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

  void reset() {
    lastSketchName.clear();
    createSketchCallCount = 0;
    lastCreatedSketch = nullptr;
    mockCreateSketchResult = true;

    lastPlaneEntityId.clear();
    createSketchOnPlaneCallCount = 0;
    mockCreateSketchOnPlaneResult = true;

    lastTargetSurfaceEntityId.clear();
    createSketchInTargetComponentCallCount = 0;
    mockCreateSketchInTargetComponentResult = true;

    lastFindSketchName.clear();
    findSketchCallCount = 0;
    lastFoundSketch = nullptr;
    mockFindSketchResult = false;

    lastExtractedEntityId.clear();
    extractProfileVerticesCallCount = 0;
    mockExtractProfileVerticesResult = true;
    mockProfileVertices.clear();
    mockSketchPlaneZ = 0.0;

    lastExtractedPlaneProfileId.clear();
    extractPlaneCallCount = 0;
    mockPlaneEntityId.clear();

    lastQueriedSurfaceId.clear();
    lastQueriedX = 0.0;
    lastQueriedY = 0.0;
    getSurfaceZCallCount = 0;
    mockSurfaceZResult = false;
    mockSurfaceZ = 0.0;

    getAllSketchNamesCallCount = 0;
    mockSketchNames = {"Imported Design", "V-Carve Toolpaths - 90° V-bit", "Test Sketch"};

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
  }
};

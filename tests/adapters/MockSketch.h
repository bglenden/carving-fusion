/**
 * MockSketch.h
 * Mock sketch for testing - captures sketch operations for verification
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "adapters/IFusionInterface.h"
#include "geometry/Point3D.h"
#include "geometry/Shape.h"

using namespace ChipCarving::Adapters;

class MockSketch : public ISketch {
 public:
  explicit MockSketch(const std::string& name) : name_(name) {}

  void addShape(const ChipCarving::Geometry::Shape* shape, ILogger* logger = nullptr) override {
    addedShapes.push_back(shape);
    addShapeCallCount++;

    if (shape) {
      shape->drawToSketch(this, logger);
    }
  }

  std::string getName() const override { return name_; }

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
      return static_cast<int>(points.size()) - 1;
    }
    return -1;
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

  void finishSketch() override { finishSketchCallCount++; }

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
  bool addSpline3D(const std::vector<ChipCarving::Geometry::Point3D>& pts) override {
    if (pts.size() < 2) {
      return false;
    }
    splines3D.push_back(pts);
    return true;
  }

  bool addLine3D(double x1, double y1, double z1, double x2, double y2, double z2) override {
    lines3D.push_back({ChipCarving::Geometry::Point3D(x1, y1, z1),
                       ChipCarving::Geometry::Point3D(x2, y2, z2)});
    return true;
  }

  bool addPoint3D(double x, double y, double z) override {
    points3D.push_back(ChipCarving::Geometry::Point3D(x, y, z));
    return true;
  }

  std::vector<std::string> getSketchCurveEntityIds() override { return mockCurveEntityIds; }

  // Test helper structs
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

  // Test state
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

  std::vector<Line> constructionLines;
  std::vector<Circle> constructionCircles;
  std::vector<Point> constructionPoints;
  int clearConstructionGeometryCallCount = 0;

  std::vector<std::vector<ChipCarving::Geometry::Point3D>> splines3D;
  std::vector<std::pair<ChipCarving::Geometry::Point3D, ChipCarving::Geometry::Point3D>> lines3D;
  std::vector<ChipCarving::Geometry::Point3D> points3D;

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

    constructionLines.clear();
    constructionCircles.clear();
    constructionPoints.clear();
    clearConstructionGeometryCallCount = 0;

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

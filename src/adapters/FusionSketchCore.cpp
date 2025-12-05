/**
 * FusionSketchCore.cpp
 *
 * Core functionality for FusionSketch - constructor, basic sketch operations,
 * 2D geometry Split from FusionSketch.cpp for maintainability
 */

#include <cmath>

#include "FusionAPIAdapter.h"
#include "geometry/Shape.h"
#include "utils/UnitConversion.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using adsk::core::Application;
using adsk::core::Point3D;
using adsk::core::Ptr;

namespace ChipCarving {
namespace Adapters {

// FusionSketch Implementation
FusionSketch::FusionSketch(const std::string& name, const Ptr<Application>& app,
                           const Ptr<adsk::fusion::Sketch>& sketch)
    : name_(name), app_(app), sketch_(sketch) {}

void FusionSketch::addShape(const Geometry::Shape* shape, ILogger* logger) {
  // Debug logging disabled for performance
  // if (logger) {
  //     logger->logDebug("FusionSketch::addShape called");
  // }
  if (shape) {
    // if (logger) {
    //     logger->logDebug("Shape is valid, calling drawToSketch");
    // }
    shape->drawToSketch(this, logger);
    // if (logger) {
    //     logger->logDebug("drawToSketch completed");
    // }
  } else {
    // if (logger) {
    //     logger->logWarning("Shape is null!");
    // }
  }
}

std::string FusionSketch::getName() const {
  return name_;
}

bool FusionSketch::addLineToSketch(double x1, double y1, double x2, double y2) {
  if (!sketch_) {
    return false;
  }

  Ptr<adsk::fusion::SketchLines> lines = sketch_->sketchCurves()->sketchLines();
  if (!lines) {
    return false;
  }

  // Create start and end points (convert from mm to Fusion's database units -
  // cm)
  Ptr<Point3D> startPoint = Point3D::create(Utils::mmToFusionLength(x1), Utils::mmToFusionLength(y1), 0);
  Ptr<Point3D> endPoint = Point3D::create(Utils::mmToFusionLength(x2), Utils::mmToFusionLength(y2), 0);

  Ptr<adsk::fusion::SketchLine> line = lines->addByTwoPoints(startPoint, endPoint);

  return line != nullptr;
}

bool FusionSketch::addArcToSketch(double centerX, double centerY, double radius, double startAngle, double endAngle) {
  if (!sketch_) {
    return false;
  }

  Ptr<adsk::fusion::SketchArcs> arcs = sketch_->sketchCurves()->sketchArcs();
  if (!arcs) {
    return false;
  }

  // Convert angles from degrees to radians and adjust for coordinate system
  double startRad = startAngle * M_PI / 180.0;
  double endRad = endAngle * M_PI / 180.0;

  // Create center point (convert from mm to Fusion's database units - cm)
  Ptr<Point3D> centerPoint = Point3D::create(Utils::mmToFusionLength(centerX), Utils::mmToFusionLength(centerY), 0);

  // Create start and end points on the arc (convert from mm to cm)
  double fusionRadius = Utils::mmToFusionLength(radius);
  double startX = Utils::mmToFusionLength(centerX) + fusionRadius * cos(startRad);
  double startY = Utils::mmToFusionLength(centerY) + fusionRadius * sin(startRad);
  double endX = Utils::mmToFusionLength(centerX) + fusionRadius * cos(endRad);
  double endY = Utils::mmToFusionLength(centerY) + fusionRadius * sin(endRad);

  Ptr<Point3D> startPoint = Point3D::create(startX, startY, 0);
  Ptr<Point3D> endPoint = Point3D::create(endX, endY, 0);

  // Add arc by center and two points
  Ptr<adsk::fusion::SketchArc> arc = arcs->addByCenterStartEnd(centerPoint, startPoint, endPoint);

  return arc != nullptr;
}

int FusionSketch::addPointToSketch(double x, double y) {
  if (!sketch_) {
    return -1;
  }

  Ptr<adsk::fusion::SketchPoints> points = sketch_->sketchPoints();
  if (!points) {
    return -1;
  }

  // Create point (convert from mm to Fusion's database units - cm)
  Ptr<Point3D> point = Point3D::create(Utils::mmToFusionLength(x), Utils::mmToFusionLength(y), 0);
  Ptr<adsk::fusion::SketchPoint> sketchPoint = points->add(point);

  if (!sketchPoint) {
    return -1;
  }

  // Store the SketchPoint object and return its index
  sketchPoints_.push_back(sketchPoint);
  return static_cast<int>(sketchPoints_.size() - 1);
}

bool FusionSketch::addArcByThreePointsToSketch(int startPointIndex, int midPointIndex, int endPointIndex) {
  if (!sketch_) {
    return false;
  }

  // Validate indices are within bounds
  if (startPointIndex >= static_cast<int>(sketchPoints_.size()) ||
      midPointIndex >= static_cast<int>(sketchPoints_.size()) ||
      endPointIndex >= static_cast<int>(sketchPoints_.size()) || startPointIndex < 0 || midPointIndex < 0 ||
      endPointIndex < 0) {
    return false;
  }

  Ptr<adsk::fusion::SketchArcs> arcs = sketch_->sketchCurves()->sketchArcs();
  if (!arcs) {
    return false;
  }

  // Get the SketchPoint objects
  Ptr<adsk::fusion::SketchPoint> startPt = sketchPoints_[startPointIndex];
  Ptr<adsk::fusion::SketchPoint> midPt = sketchPoints_[midPointIndex];
  Ptr<adsk::fusion::SketchPoint> endPt = sketchPoints_[endPointIndex];

  if (!startPt || !midPt || !endPt) {
    return false;
  }

  // Create arc by three points using Fusion API:
  // startPoint: SketchPoint (for constraint), point: Point3D (geometry),
  // endPoint: SketchPoint (for constraint)
  Ptr<adsk::core::Point3D> midPoint3D = midPt->geometry();
  Ptr<adsk::fusion::SketchArc> arc = arcs->addByThreePoints(startPt, midPoint3D, endPt);

  return arc != nullptr;
}

bool FusionSketch::addLineByTwoPointsToSketch(int startPointIndex, int endPointIndex) {
  if (!sketch_ || startPointIndex >= static_cast<int>(sketchPoints_.size()) ||
      endPointIndex >= static_cast<int>(sketchPoints_.size()) || startPointIndex < 0 || endPointIndex < 0) {
    return false;
  }

  Ptr<adsk::fusion::SketchLines> lines = sketch_->sketchCurves()->sketchLines();
  if (!lines) {
    return false;
  }

  // Get the SketchPoint objects
  Ptr<adsk::fusion::SketchPoint> startPt = sketchPoints_[startPointIndex];
  Ptr<adsk::fusion::SketchPoint> endPt = sketchPoints_[endPointIndex];

  if (!startPt || !endPt) {
    return false;
  }

  // Create line by two points
  Ptr<adsk::fusion::SketchLine> line = lines->addByTwoPoints(startPt, endPt);

  return line != nullptr;
}

bool FusionSketch::deleteSketchPoint(int pointIndex) {
  if (!sketch_ || pointIndex < 0 || pointIndex >= static_cast<int>(sketchPoints_.size())) {
    return false;
  }

  Ptr<adsk::fusion::SketchPoint> point = sketchPoints_[pointIndex];
  if (!point) {
    return false;
  }

  // IMPORTANT: Check isValid() before deleteMe()
  // Objects can become invalid after undo operations or model changes.
  // Calling deleteMe() on an invalid object can cause crashes.
  if (!point->isValid()) {
    // Already deleted or invalidated - just remove from our tracking
    sketchPoints_.erase(sketchPoints_.begin() + pointIndex);
    return true;
  }

  // Delete the point from the sketch
  bool result = point->deleteMe();

  if (result) {
    // Remove from our tracking vector
    sketchPoints_.erase(sketchPoints_.begin() + pointIndex);
  }

  return result;
}

void FusionSketch::finishSketch() {
  // NOTE: Fusion API sketches don't require explicit "finishing" - geometry is
  // committed immediately when created. This method exists for interface
  // compatibility and potential future use.
  //
  // HISTORICAL: Previously used executeTextCommand("Commands.Start3DSketch/Stop3DSketch")
  // to "flush pending operations", but this pattern is:
  // 1. Not documented as necessary for 2D sketches
  // 2. Uses unstable text commands that can change between Fusion versions
  // 3. Adds unnecessary overhead
  //
  // If sketch geometry issues arise, consider adding proper API calls here
  // rather than text command workarounds.

  // Currently a no-op - sketch geometry is committed on creation
  (void)sketch_;  // Suppress unused warning
}

}  // namespace Adapters
}  // namespace ChipCarving

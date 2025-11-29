/**
 * FusionSketchConstruction.cpp
 *
 * Construction geometry functionality for FusionSketch
 * Split from FusionSketch.cpp for maintainability
 */

#include <cmath>
#include <iostream>

#include "geometry/Point3D.h"
#include "geometry/Shape.h"
#include "utils/logging.h"
#include "utils/UnitConversion.h"
#include "FusionAPIAdapter.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

bool FusionSketch::addConstructionLine(double x1, double y1, double x2, double y2) {
  if (!sketch_) {
    return false;
  }

  try {
    // Get sketch lines collection
    Ptr<adsk::fusion::SketchLines> lines = sketch_->sketchCurves()->sketchLines();
    if (!lines) {
      return false;
    }

    // Debug: Log if sketch is on non-XY plane
    if (sketch_->referencePlane()) {
      LOG_DEBUG("Adding construction line to sketch on reference plane");
      LOG_DEBUG("  Line coords (mm): (" << x1 << ", " << y1 << ") to (" << x2 << ", " << y2 << ")");
    }

    // Create start and end points (convert from mm to Fusion's database units -
    // cm)
    Ptr<Point3D> startPoint = Point3D::create(Utils::mmToFusionLength(x1), Utils::mmToFusionLength(y1), 0);
    Ptr<Point3D> endPoint = Point3D::create(Utils::mmToFusionLength(x2), Utils::mmToFusionLength(y2), 0);

    // Add line to sketch
    Ptr<adsk::fusion::SketchLine> line = lines->addByTwoPoints(startPoint, endPoint);
    if (!line) {
      return false;
    }

    // Make it construction geometry
    line->isConstruction(true);

    // Store for later cleanup
    constructionLines_.push_back(line);

    return true;
  } catch (const std::exception& e) {
    std::cout << "Construction line error: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cout << "Unknown construction line error" << std::endl;
    return false;
  }
}

bool FusionSketch::addConstructionCircle(double centerX, double centerY, double radius) {
  if (!sketch_) {
    return false;
  }

  try {
    // Get sketch circles collection
    Ptr<adsk::fusion::SketchCircles> circles = sketch_->sketchCurves()->sketchCircles();
    if (!circles) {
      return false;
    }

    // Create center point (convert from mm to Fusion's database units - cm)
    Ptr<Point3D> centerPoint = Point3D::create(Utils::mmToFusionLength(centerX), Utils::mmToFusionLength(centerY), 0);

    // Add circle to sketch (convert radius from mm to cm)
    Ptr<adsk::fusion::SketchCircle> circle = circles->addByCenterRadius(centerPoint, Utils::mmToFusionLength(radius));
    if (!circle) {
      return false;
    }

    // Make it construction geometry
    circle->isConstruction(true);

    // Store for later cleanup
    constructionCircles_.push_back(circle);

    return true;
  } catch (const std::exception& e) {
    std::cout << "Construction circle error: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cout << "Unknown construction circle error" << std::endl;
    return false;
  }
}

bool FusionSketch::addConstructionPoint(double x, double y) {
  if (!sketch_) {
    return false;
  }

  try {
    // Get sketch points collection
    Ptr<adsk::fusion::SketchPoints> points = sketch_->sketchPoints();
    if (!points) {
      return false;
    }

    // Create point (convert from mm to Fusion's database units - cm)
    Ptr<Point3D> point = Point3D::create(Utils::mmToFusionLength(x), Utils::mmToFusionLength(y), 0);
    Ptr<adsk::fusion::SketchPoint> sketchPoint = points->add(point);
    if (!sketchPoint) {
      return false;
    }

    // Note: SketchPoint doesn't have isConstruction method
    // Construction points are handled differently in Fusion API

    // Store for later cleanup
    constructionPoints_.push_back(sketchPoint);

    return true;
  } catch (const std::exception& e) {
    std::cout << "Construction point error: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cout << "Unknown construction point error" << std::endl;
    return false;
  }
}

void FusionSketch::clearConstructionGeometry() {
  // PATTERN: Always check isValid() before deleteMe()
  // Fusion API objects can become invalid after undo operations, document
  // changes, or if another operation deleted them. Calling deleteMe() on
  // an invalid object can cause crashes or undefined behavior.
  try {
    // Clear construction lines
    for (auto& line : constructionLines_) {
      if (line && line->isValid()) {
        line->deleteMe();
      }
    }
    constructionLines_.clear();

    // Clear construction circles
    for (auto& circle : constructionCircles_) {
      if (circle && circle->isValid()) {
        circle->deleteMe();
      }
    }
    constructionCircles_.clear();

    // Clear construction points
    for (auto& point : constructionPoints_) {
      if (point && point->isValid()) {
        point->deleteMe();
      }
    }
    constructionPoints_.clear();
  } catch (const std::exception& e) {
    std::cout << "Clear construction geometry error: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "Unknown clear construction geometry error" << std::endl;
  }
}

}  // namespace Adapters
}  // namespace ChipCarving

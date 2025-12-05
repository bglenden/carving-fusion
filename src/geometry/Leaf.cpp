/**
 * Leaf shape implementation
 * Ported from TypeScript design_program/src/shapes/Leaf.ts
 */

#include "geometry/Leaf.h"

#include <algorithm>
#include <cmath>

#include "adapters/IFusionInterface.h"

using ChipCarving::Geometry::Leaf;
using ChipCarving::Geometry::Point2D;

std::pair<Point2D, Point2D> Leaf::getArcCenters() const {
  const Point2D mid = midpoint(focus1_, focus2_);
  const Point2D perpVec = perpendicular(focus1_, focus2_);
  const double d_center = getChordCenterToArcCenterDistance();

  // Two arc centers: one offset in each direction from chord midpoint
  Point2D center1(mid.x + d_center * perpVec.x, mid.y + d_center * perpVec.y);
  Point2D center2(mid.x - d_center * perpVec.x, mid.y - d_center * perpVec.y);

  return std::make_pair(center1, center2);
}

std::pair<Leaf::ArcParams, Leaf::ArcParams> Leaf::getArcParameters() const {
  if (!isValidGeometry()) {
    // Return degenerate arcs for invalid geometry
    Point2D mid = midpoint(focus1_, focus2_);
    ArcParams degenerate(mid, 0.0, 0.0, 0.0, false);
    return std::make_pair(degenerate, degenerate);
  }

  auto centers = getArcCenters();
  Point2D center1 = centers.first;
  Point2D center2 = centers.second;

  // Calculate start and end angles for first arc (focus1 to focus2)
  double startAngle1 = std::atan2(focus1_.y - center1.y, focus1_.x - center1.x);
  double endAngle1 = std::atan2(focus2_.y - center1.y, focus2_.x - center1.x);

  // Calculate start and end angles for second arc (focus2 to focus1)
  double startAngle2 = std::atan2(focus2_.y - center2.y, focus2_.x - center2.x);
  double endAngle2 = std::atan2(focus1_.y - center2.y, focus1_.x - center2.x);

  // Both arcs should be drawn as minor arcs
  // For a vesica piscis, we always want the inner (shorter) arc segment
  bool anticlockwise1 = false;  // Will be calculated based on angle difference
  bool anticlockwise2 = false;

  // Calculate angle differences to determine arc direction
  double angleDiff1 = endAngle1 - startAngle1;
  if (angleDiff1 > M_PI)
    angleDiff1 -= 2 * M_PI;
  if (angleDiff1 < -M_PI)
    angleDiff1 += 2 * M_PI;
  anticlockwise1 = angleDiff1 < 0;

  double angleDiff2 = endAngle2 - startAngle2;
  if (angleDiff2 > M_PI)
    angleDiff2 -= 2 * M_PI;
  if (angleDiff2 < -M_PI)
    angleDiff2 += 2 * M_PI;
  anticlockwise2 = angleDiff2 < 0;

  ArcParams arc1(center1, radius_, startAngle1, endAngle1, anticlockwise1);
  ArcParams arc2(center2, radius_, startAngle2, endAngle2, anticlockwise2);

  return std::make_pair(arc1, arc2);
}

double Leaf::getSagitta() const {
  const double dist = distance(focus1_, focus2_);
  if (dist < 1e-9 || dist > 2 * radius_) {
    return 0.0;
  }

  const double halfChord = dist / 2.0;
  const double d_center = std::sqrt(std::max(0.0, radius_ * radius_ - halfChord * halfChord));
  return radius_ - d_center;
}

bool Leaf::isValidGeometry() const {
  const double dist = distance(focus1_, focus2_);
  return dist >= 1e-9 && dist <= 2 * radius_;
}

void Leaf::drawToSketch(Adapters::ISketch* sketch, Adapters::ILogger* logger) const {
  (void)logger;  // Suppress unused parameter warning
  if (!sketch || !isValidGeometry()) {
    return;
  }

  // Debug logging disabled for performance
  // if (logger) {
  //     logger->logDebug("Leaf::drawToSketch called");
  // }

  // First, add the two focus points as sketch points
  int focus1Idx = sketch->addPointToSketch(focus1_.x, focus1_.y);
  int focus2Idx = sketch->addPointToSketch(focus2_.x, focus2_.y);

  if (focus1Idx < 0 || focus2Idx < 0) {
    // Error logging disabled for performance
    // if (logger) {
    //     logger->logError("Failed to add focus points to sketch");
    // }
    return;
  }

  auto arcParams = getArcParameters();

  // For each arc, calculate midpoint and draw using three points
  const auto& arc1 = arcParams.first;
  const auto& arc2 = arcParams.second;

  std::vector<int> midpointsToDelete;  // Track midpoints for cleanup

  // Calculate midpoint for first arc - ensure we get the shorter arc
  // This fixes the issue where some leaf shapes appeared as crescents instead
  // of footballs
  double angleDiff1 = arc1.endAngle - arc1.startAngle;
  while (angleDiff1 > M_PI)
    angleDiff1 -= 2 * M_PI;
  while (angleDiff1 < -M_PI)
    angleDiff1 += 2 * M_PI;

  double midAngle1;
  if (std::abs(angleDiff1) <= M_PI) {
    // Normal case: shortest path is direct
    midAngle1 = arc1.startAngle + angleDiff1 / 2.0;
  } else {
    // Wrapping case: go the other way around
    if (angleDiff1 > 0) {
      midAngle1 = arc1.startAngle + (angleDiff1 - 2 * M_PI) / 2.0;
    } else {
      midAngle1 = arc1.startAngle + (angleDiff1 + 2 * M_PI) / 2.0;
    }
  }

  double midX1 = arc1.center.x + arc1.radius * cos(midAngle1);
  double midY1 = arc1.center.y + arc1.radius * sin(midAngle1);
  int mid1Idx = sketch->addPointToSketch(midX1, midY1);

  // Calculate midpoint for second arc - ensure we get the shorter arc
  double angleDiff2 = arc2.endAngle - arc2.startAngle;
  while (angleDiff2 > M_PI)
    angleDiff2 -= 2 * M_PI;
  while (angleDiff2 < -M_PI)
    angleDiff2 += 2 * M_PI;

  double midAngle2;
  if (std::abs(angleDiff2) <= M_PI) {
    // Normal case: shortest path is direct
    midAngle2 = arc2.startAngle + angleDiff2 / 2.0;
  } else {
    // Wrapping case: go the other way around
    if (angleDiff2 > 0) {
      midAngle2 = arc2.startAngle + (angleDiff2 - 2 * M_PI) / 2.0;
    } else {
      midAngle2 = arc2.startAngle + (angleDiff2 + 2 * M_PI) / 2.0;
    }
  }

  double midX2 = arc2.center.x + arc2.radius * cos(midAngle2);
  double midY2 = arc2.center.y + arc2.radius * sin(midAngle2);
  int mid2Idx = sketch->addPointToSketch(midX2, midY2);

  if (mid1Idx < 0 || mid2Idx < 0) {
    // Error logging disabled for performance
    // if (logger) {
    //     logger->logError("Failed to add arc midpoints to sketch");
    // }
    return;
  }

  // Draw first arc: focus1 -> midpoint1 -> focus2
  bool arc1Created = sketch->addArcByThreePointsToSketch(focus1Idx, mid1Idx, focus2Idx);
  if (arc1Created) {
    midpointsToDelete.push_back(mid1Idx);
  }

  // Draw second arc: focus2 -> midpoint2 -> focus1
  bool arc2Created = sketch->addArcByThreePointsToSketch(focus2Idx, mid2Idx, focus1Idx);
  if (arc2Created) {
    midpointsToDelete.push_back(mid2Idx);
  }

  // Clean up midpoints after all arcs are created
  // Delete in reverse order to maintain valid indices
  for (int i = static_cast<int>(midpointsToDelete.size()) - 1; i >= 0; --i) {
    sketch->deleteSketchPoint(midpointsToDelete[i]);
  }
}

bool Leaf::contains(const Point2D& point) const {
  if (!isValidGeometry()) {
    return false;
  }

  // Point is inside vesica piscis if it's inside BOTH circles
  double dist1 = distance(point, focus1_);
  double dist2 = distance(point, focus2_);

  return (dist1 <= radius_) && (dist2 <= radius_);
}

Point2D Leaf::getCentroid() const {
  // Geometric center is the midpoint between foci
  return midpoint(focus1_, focus2_);
}

double Leaf::getChordCenterToArcCenterDistance() const {
  const double dist = distance(focus1_, focus2_);
  if (dist < 1e-9 || dist > 2 * radius_) {
    return 0.0;
  }

  const double halfChord = dist / 2.0;
  return std::sqrt(std::max(0.0, radius_ * radius_ - halfChord * halfChord));
}

// NOTE: Polygon vertices are now extracted directly from Fusion geometry using
// strokes This method has been removed because it incorrectly used original
// shape parameters instead of actual Fusion geometry. The polygonization is now
// handled by FusionAPIAdapter::extractProfileVertices() using Fusion's strokes
// API.

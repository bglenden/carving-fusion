/**
 * TriArcGeometry.cpp
 *
 * Geometric calculations for TriArc - arc parameters, angles, centers, etc.
 * Split from TriArc.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

#include "../../include/geometry/TriArc.h"
#include "../../src/adapters/IFusionInterface.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace ChipCarving::Geometry;

std::array<ArcParams, 3> TriArc::getArcParameters() const {
  std::array<ArcParams, 3> arcs;
  for (int i = 0; i < 3; ++i) {
    arcs[i] = getArcParameters(i);
  }
  return arcs;
}

ArcParams TriArc::getArcParameters(int arcIndex) const {
  if (arcIndex < 0 || arcIndex >= 3) {
    throw std::out_of_range("Arc index must be 0, 1, or 2");
  }

  // Get edge vertices
  Point2D p1 = vertices_[arcIndex];
  Point2D p2 = vertices_[(arcIndex + 1) % 3];
  double bulgeFactor = bulgeFactors_[arcIndex];

  // Check for straight edge
  if (isEdgeStraight(arcIndex)) {
    return ArcParams();  // Return default (invalid) arc parameters
  }

  // Calculate arc center
  Point2D center = calculateArcCenter(p1, p2, bulgeFactor, arcIndex);

  // Calculate radius
  double chordLength = getChordLength(arcIndex);
  double sagitta = sagittaFromBulge(bulgeFactor, chordLength);
  double radius = calculateRadius(chordLength, sagitta);

  // Calculate angles
  bool isConcave = bulgeFactor < 0;
  auto angles = calculateArcAngles(center, p1, p2, isConcave);

  // For concave arcs, use cross product to determine correct sweep direction
  bool anticlockwise = !isConcave;  // Default for convex arcs
  if (isConcave) {
    // Use cross product to determine if we should sweep clockwise or
    // counterclockwise Vector from center to p1
    Point2D centerToP1(p1.x - center.x, p1.y - center.y);
    // Vector from center to p2
    Point2D centerToP2(p2.x - center.x, p2.y - center.y);

    // Cross product (2D): positive means counterclockwise from p1 to p2
    double cross = centerToP1.x * centerToP2.y - centerToP1.y * centerToP2.x;

    // For concave arcs, we want the arc that curves inward toward triangle
    // center Since our center is positioned away from triangle center, we want
    // the smaller arc
    anticlockwise = (cross > 0);
  }

  return ArcParams(center, radius, angles.first, angles.second, anticlockwise);
}

double TriArc::sagittaFromBulge(double bulge, double chordLength) {
  return std::abs(bulge * chordLength) / 2.0;
}

double TriArc::bulgeFromSagitta(double sagitta, double chordLength) {
  if (chordLength < EPSILON) {
    return 0.0;
  }
  return -(2.0 * sagitta) / chordLength;  // Negative for concave
}

Point2D TriArc::getChordMidpoint(int arcIndex) const {
  if (arcIndex < 0 || arcIndex >= 3) {
    throw std::out_of_range("Arc index must be 0, 1, or 2");
  }

  Point2D p1 = vertices_[arcIndex];
  Point2D p2 = vertices_[(arcIndex + 1) % 3];
  return midpoint(p1, p2);
}

Point2D TriArc::getPerpendicularNormal(int arcIndex) const {
  if (arcIndex < 0 || arcIndex >= 3) {
    throw std::out_of_range("Arc index must be 0, 1, or 2");
  }

  Point2D p1 = vertices_[arcIndex];
  Point2D p2 = vertices_[(arcIndex + 1) % 3];

  // Calculate edge vector
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  double length = std::sqrt(dx * dx + dy * dy);

  if (length < EPSILON) {
    return Point2D(0, 0);  // Degenerate edge
  }

  // Calculate both possible perpendicular directions
  Point2D normal1(-dy / length, dx / length);  // Left-hand rule
  Point2D normal2(dy / length, -dx / length);  // Right-hand rule

  // For CONCAVE arcs (negative bulge), choose normal that points AWAY from
  // triangle centroid
  Point2D chordMid = getChordMidpoint(arcIndex);
  Point2D centroid = getCenter();
  Point2D toCentroid = centroid - chordMid;

  // Check which normal points away from centroid (negative dot product)
  double dot1 = normal1.x * toCentroid.x + normal1.y * toCentroid.y;
  double dot2 = normal2.x * toCentroid.x + normal2.y * toCentroid.y;

  // For concave arcs, we want the normal that points AWAY from centroid (more
  // negative dot product)
  if (dot1 < dot2) {
    return normal1;  // normal1 points more away from centroid
  } else {
    return normal2;  // normal2 points more away from centroid
  }
}

bool TriArc::hasValidBulgeFactors() const {
  return std::all_of(
      bulgeFactors_.begin(), bulgeFactors_.end(), [](double bulge) {
        return bulge <= 0 && bulge >= MIN_BULGE && bulge <= MAX_BULGE;
      });
}

void TriArc::clampBulgeFactors() {
  for (double& bulge : bulgeFactors_) {
    // Allow very small values to remain for "straight" edges
    if (std::abs(bulge) < EPSILON) {
      // Keep very small values as-is for straight edges
      continue;
    }
    bulge = std::max(MIN_BULGE, std::min(MAX_BULGE, bulge));
  }
}

bool TriArc::isEdgeStraight(int arcIndex) const {
  if (arcIndex < 0 || arcIndex >= 3) {
    return true;  // Invalid index treated as straight
  }

  return std::abs(bulgeFactors_[arcIndex]) < EPSILON;
}

double TriArc::getChordLength(int arcIndex) const {
  if (arcIndex < 0 || arcIndex >= 3) {
    return 0.0;
  }

  Point2D p1 = vertices_[arcIndex];
  Point2D p2 = vertices_[(arcIndex + 1) % 3];
  return distance(p1, p2);
}

Point2D TriArc::calculateArcCenter(const Point2D& p1, const Point2D& p2,
                                   double bulgeFactor, int arcIndex) const {
  double chordLength = distance(p1, p2);
  if (chordLength < EPSILON) {
    return midpoint(p1, p2);  // Degenerate edge
  }

  double sagitta = sagittaFromBulge(bulgeFactor, chordLength);
  double radius = calculateRadius(chordLength, sagitta);
  double distMidToCenter = radius - sagitta;

  Point2D chordMid = midpoint(p1, p2);

  // Use the getPerpendicularNormal method which already handles centroid
  // direction
  Point2D normal = getPerpendicularNormal(arcIndex);

  // Position center at correct distance along normal
  return Point2D(chordMid.x + normal.x * distMidToCenter,
                 chordMid.y + normal.y * distMidToCenter);
}

double TriArc::calculateRadius(double chordLength, double sagitta) {
  if (sagitta < EPSILON) {
    return std::numeric_limits<double>::infinity();  // Straight line
  }

  // Standard circular arc formula: r = (h/2) + (c²)/(8h)
  // where h = sagitta, c = chord length
  return (sagitta / 2.0) + (chordLength * chordLength) / (8.0 * sagitta);
}

std::pair<double, double> TriArc::calculateArcAngles(const Point2D& center,
                                                     const Point2D& p1,
                                                     const Point2D& p2,
                                                     bool /* isConcave */) {
  // Calculate angles from center to both points
  double startAngle = std::atan2(p1.y - center.y, p1.x - center.x);
  double endAngle = std::atan2(p2.y - center.y, p2.x - center.x);

  // atan2 returns values in [-π, π] range
  // We need to ensure we get the smaller arc (not the larger complementary arc)
  // This is critical for Fusion API which will draw from start to end point

  return std::make_pair(startAngle, endAngle);
}

// NOTE: Polygon vertices are now extracted directly from Fusion geometry using
// strokes This method has been removed because it incorrectly used original
// bulge factors instead of actual Fusion geometry. The polygonization is now
// handled by FusionAPIAdapter::extractProfileVertices() which uses actual
// geometric curves from Fusion.

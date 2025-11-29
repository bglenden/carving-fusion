/**
 * TriArcSketch.cpp
 *
 * Sketch drawing functionality for TriArc
 * Split from TriArc.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

#include "geometry/TriArc.h"
#include "adapters/IFusionInterface.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace ChipCarving::Geometry;

void TriArc::drawToSketch(Adapters::ISketch* sketch, Adapters::ILogger* logger) const {
  (void)logger;  // Suppress unused parameter warning
  if (!sketch) {
    return;
  }

  // Debug logging disabled for performance
  // if (logger) {
  //     logger->logDebug("TriArc::drawToSketch called");
  // }

  // First, add the three vertices as sketch points
  std::vector<int> vertexIndices;
  for (int i = 0; i < 3; ++i) {
    int pointIndex = sketch->addPointToSketch(vertices_[i].x, vertices_[i].y);
    if (pointIndex < 0) {
      // Error logging disabled for performance
      // if (logger) {
      //     logger->logError("Failed to add vertex point " +
      //     std::to_string(i));
      // }
      return;  // Exit if point creation fails
    }
    vertexIndices.push_back(pointIndex);  // Store the actual point index
  }

  // Draw each edge using the vertex points
  std::vector<int> midpointsToDelete;  // Track midpoints for cleanup

  for (int i = 0; i < 3; ++i) {
    int startIdx = vertexIndices[i];
    int endIdx = vertexIndices[(i + 1) % 3];

    bool isStraight = isEdgeStraight(i);
    // Edge processing for TriArc shape

    if (isStraight) {
      // Draw as straight line between vertex points
      sketch->addLineByTwoPointsToSketch(startIdx, endIdx);
    } else {
      // For curved edges, calculate arc midpoint directly from bulge factor
      Point2D p1 = vertices_[i];
      Point2D p2 = vertices_[(i + 1) % 3];
      double bulgeFactor = bulgeFactors_[i];

      // Calculate chord midpoint
      Point2D chordMid = Point2D((p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0);

      // Calculate chord length and sagitta (arc height)
      double chordLength = distance(p1, p2);
      double sagitta = std::abs(bulgeFactor * chordLength) / 2.0;

      // Calculate perpendicular direction from chord midpoint
      // For concave arcs (negative bulge), the arc curves toward the triangle
      // center
      Point2D chordDir = Point2D(p2.x - p1.x, p2.y - p1.y);
      double chordLen = std::sqrt(chordDir.x * chordDir.x + chordDir.y * chordDir.y);
      if (chordLen > 1e-9) {
        chordDir.x /= chordLen;
        chordDir.y /= chordLen;
      }

      // Perpendicular vector (rotate 90 degrees)
      Point2D perpDir = Point2D(-chordDir.y, chordDir.x);

      // For concave arcs, determine which direction curves toward triangle
      // center
      Point2D triangleCenter = getCenter();
      Point2D toCenter = Point2D(triangleCenter.x - chordMid.x, triangleCenter.y - chordMid.y);

      // Choose perpendicular direction that points toward triangle center for
      // concave arcs
      double dot = perpDir.x * toCenter.x + perpDir.y * toCenter.y;
      if (dot < 0) {
        perpDir.x = -perpDir.x;
        perpDir.y = -perpDir.y;
      }

      // Calculate arc midpoint
      double midX = chordMid.x + perpDir.x * sagitta;
      double midY = chordMid.y + perpDir.y * sagitta;

      // Arc midpoint calculated for three-point construction

      // Add midpoint to sketch
      int midIdx = sketch->addPointToSketch(midX, midY);
      if (midIdx < 0) {
        // Error logging disabled for performance
        // if (logger) {
        //     logger->logError("Failed to add midpoint for edge " +
        //     std::to_string(i));
        // }
        continue;  // Skip this edge if midpoint creation fails
      }

      // Draw arc using three points: start vertex, midpoint, end vertex
      bool arcCreated = sketch->addArcByThreePointsToSketch(startIdx, midIdx, endIdx);

      if (arcCreated) {
        // Track midpoint for deletion (only if arc was successfully created)
        midpointsToDelete.push_back(midIdx);
      }
    }
  }

  // Clean up midpoints after all arcs are created
  // Delete in reverse order to maintain valid indices
  for (int i = midpointsToDelete.size() - 1; i >= 0; --i) {
    sketch->deleteSketchPoint(midpointsToDelete[i]);
  }
}

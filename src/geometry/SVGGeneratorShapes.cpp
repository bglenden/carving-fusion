/**
 * SVGGeneratorShapes.cpp
 *
 * Shape-specific SVG generation - Leaf and TriArc rendering with debug markers.
 * Split from SVGGenerator.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <regex>

#include "geometry/SVGGenerator.h"

using ChipCarving::Geometry::SVGGenerator;
using ChipCarving::Geometry::Point2D;
using ChipCarving::Geometry::Leaf;
using ChipCarving::Geometry::TriArc;
using ChipCarving::Geometry::ArcParams;

void SVGGenerator::addLeaf(const Leaf& leaf, const std::string& color, double strokeWidth) {
  if (!leaf.isValidGeometry()) {
    // Draw as a simple line for invalid geometry
    Point2D f1 = worldToSVG(leaf.getFocus1());
    Point2D f2 = worldToSVG(leaf.getFocus2());
    svg_ << "  <line x1=\"" << f1.x << "\" y1=\"" << f1.y << "\" x2=\"" << f2.x << "\" y2=\"" << f2.y << "\" stroke=\""
         << color << "\" stroke-width=\"" << strokeWidth << "\" stroke-dasharray=\"5,5\"/>\n";
    return;
  }

  auto arcParams = leaf.getArcParameters();
  auto arc1 = arcParams.first;
  auto arc2 = arcParams.second;

  // Convert to SVG coordinates
  Point2D f1_svg = worldToSVG(leaf.getFocus1());
  // Note: f2_svg, center1_svg, center2_svg not currently used but may be needed
  // for debugging
  double radius_svg = worldToSVG(arc1.radius);

  // Create SVG path for the vesica piscis
  svg_ << "  <path d=\"";

  // Move to first focus
  svg_ << "M " << f1_svg.x << "," << f1_svg.y << " ";

  // First arc from f1 to f2
  Point2D arcEnd1 = worldToSVG(leaf.getFocus2());
  svg_ << "A " << radius_svg << "," << radius_svg << " 0 0," << (arc1.anticlockwise ? "0" : "1") << " " << arcEnd1.x
       << "," << arcEnd1.y << " ";

  // Second arc from f2 back to f1
  Point2D arcEnd2 = worldToSVG(leaf.getFocus1());
  svg_ << "A " << radius_svg << "," << radius_svg << " 0 0," << (arc2.anticlockwise ? "0" : "1") << " " << arcEnd2.x
       << "," << arcEnd2.y;

  svg_ << "\" stroke=\"" << color << "\" stroke-width=\"" << strokeWidth << "\" fill=\"none\"/>\n";
}

void SVGGenerator::addTriArc(const TriArc& triArc, const std::string& color, double strokeWidth) {
  // Create SVG path for the curved triangle
  svg_ << "  <path d=\"";

  // Get all arc parameters
  auto arcParams = triArc.getArcParameters();

  // Start at first vertex
  Point2D v0_svg = worldToSVG(triArc.getVertex(0));
  svg_ << "M " << v0_svg.x << "," << v0_svg.y << " ";

  // Draw each edge (either arc or straight line)
  for (int i = 0; i < 3; ++i) {
    Point2D vNext = triArc.getVertex((i + 1) % 3);
    Point2D vNext_svg = worldToSVG(vNext);

    if (triArc.isEdgeStraight(i)) {
      // Draw straight line
      svg_ << "L " << vNext_svg.x << "," << vNext_svg.y << " ";
    } else {
      // Draw arc
      ArcParams arc = arcParams[i];
      double radius_svg = worldToSVG(arc.radius);

      // For concave arcs, always use small arc (large-arc-flag=0)
      svg_ << "A " << radius_svg << "," << radius_svg << " 0 0," << (arc.anticlockwise ? "0" : "1") << " "
           << vNext_svg.x << "," << vNext_svg.y << " ";
    }
  }

  // Close the path
  svg_ << "Z";

  svg_ << "\" stroke=\"" << color << "\" stroke-width=\"" << strokeWidth << "\" fill=\"none\"/>\n";
}

void SVGGenerator::addTriArcDebugMarkers(const TriArc& triArc) {
  // Add vertices
  for (int i = 0; i < 3; ++i) {
    Point2D vertex = triArc.getVertex(i);
    std::string label = "V" + std::to_string(i);
    addPoint(vertex, "red", 3.0, label);
  }

  // Add centroid
  addPoint(triArc.getCenter(), "green", 2.0, "Center");

  // Add arc information for curved edges
  auto arcParams = triArc.getArcParameters();
  for (int i = 0; i < 3; ++i) {
    if (triArc.isEdgeStraight(i)) {
      // Draw straight edge as dashed line
      Point2D v1 = triArc.getVertex(i);
      Point2D v2 = triArc.getVertex((i + 1) % 3);
      addLine(v1, v2, "gray", 0.5, "stroke-dasharray=\"2,2\"");
    } else {
      // Add arc center and radius lines
      ArcParams arc = arcParams[i];
      std::string label = "C" + std::to_string(i);
      addPoint(arc.center, "blue", 2.0, label);

      // Add radius lines
      Point2D v1 = triArc.getVertex(i);
      Point2D v2 = triArc.getVertex((i + 1) % 3);
      addLine(arc.center, v1, "lightblue", 0.5, "stroke-dasharray=\"1,1\"");
      addLine(arc.center, v2, "lightblue", 0.5, "stroke-dasharray=\"1,1\"");

      // Add chord line
      addLine(v1, v2, "gray", 0.5, "stroke-dasharray=\"2,2\"");

      // Add normal vector from chord midpoint
      Point2D chordMid = triArc.getChordMidpoint(i);
      Point2D normal = triArc.getPerpendicularNormal(i);
      Point2D normalEnd = Point2D(chordMid.x + normal.x * 2.0, chordMid.y + normal.y * 2.0);
      addLine(chordMid, normalEnd, "orange", 1.0);
      addPoint(chordMid, "orange", 1.5, "M" + std::to_string(i));
    }
  }

  // Add bulge factor information
  for (int i = 0; i < 3; ++i) {
    Point2D chordMid = triArc.getChordMidpoint(i);
    double bulge = triArc.getBulgeFactor(i);
    std::string bulgeText = "b" + std::to_string(i) + "=" + std::to_string(bulge).substr(0, 5);
    addText(Point2D(chordMid.x, chordMid.y - 1.5), bulgeText, "purple", 8.0);
  }
}

void SVGGenerator::addDebugMarkers(const Leaf& leaf) {
  if (!leaf.isValidGeometry()) {
    return;
  }

  // Add focus points
  addPoint(leaf.getFocus1(), "red", 3.0, "F1");
  addPoint(leaf.getFocus2(), "red", 3.0, "F2");

  // Add arc centers
  auto centers = leaf.getArcCenters();
  addPoint(centers.first, "blue", 2.0, "C1");
  addPoint(centers.second, "blue", 2.0, "C2");

  // Add centroid
  addPoint(leaf.getCentroid(), "green", 2.0, "Mid");

  // Add chord line
  addLine(leaf.getFocus1(), leaf.getFocus2(), "gray", 0.5, "stroke-dasharray=\"2,2\"");

  // Add radius lines from centers to foci
  addLine(centers.first, leaf.getFocus1(), "lightblue", 0.5, "stroke-dasharray=\"1,1\"");
  addLine(centers.first, leaf.getFocus2(), "lightblue", 0.5, "stroke-dasharray=\"1,1\"");
  addLine(centers.second, leaf.getFocus1(), "lightblue", 0.5, "stroke-dasharray=\"1,1\"");
  addLine(centers.second, leaf.getFocus2(), "lightblue", 0.5, "stroke-dasharray=\"1,1\"");
}

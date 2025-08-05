/**
 * SVGGeneratorCore.cpp
 *
 * Core SVG generation functionality - constructors, setup, and basic drawing
 * operations. Split from SVGGenerator.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <regex>

#include "../../include/geometry/SVGGenerator.h"

using namespace ChipCarving::Geometry;

SVGGenerator::SVGGenerator(double width, double height, double scale)
    : width_(width), height_(height), scale_(scale), offset_(0.0, 0.0) {
  svg_ << std::fixed << std::setprecision(3);

  // SVG header
  svg_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  svg_ << "<svg width=\"" << width_ << "\" height=\"" << height_
       << "\" viewBox=\"0 0 " << width_ << " " << height_
       << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

  // Add grid for reference
  svg_ << "  <!-- Grid -->\n";
  for (int i = 0; i <= static_cast<int>(width_ / 20); ++i) {
    double x = i * 20;
    svg_ << "  <line x1=\"" << x << "\" y1=\"0\" x2=\"" << x << "\" y2=\""
         << height_ << "\" stroke=\"#f0f0f0\" stroke-width=\"0.5\"/>\n";
  }
  for (int i = 0; i <= static_cast<int>(height_ / 20); ++i) {
    double y = i * 20;
    svg_ << "  <line x1=\"0\" y1=\"" << y << "\" x2=\"" << width_ << "\" y2=\""
         << y << "\" stroke=\"#f0f0f0\" stroke-width=\"0.5\"/>\n";
  }

  svg_ << "  <!-- Content -->\n";
}

void SVGGenerator::setBounds(const Point2D& min, const Point2D& max,
                             double margin) {
  // Calculate the world bounds with margin
  double worldWidth = (max.x - min.x) + 2 * margin;
  double worldHeight = (max.y - min.y) + 2 * margin;

  // Calculate scale to fit in SVG canvas
  double scaleX = (width_ - 40) / worldWidth;  // 20px margin on each side
  double scaleY = (height_ - 40) / worldHeight;
  scale_ = std::min(scaleX, scaleY);

  // Calculate offset to center the content
  Point2D worldCenter((min.x + max.x) / 2.0, (min.y + max.y) / 2.0);
  Point2D svgCenter(width_ / 2.0, height_ / 2.0);

  offset_.x = svgCenter.x - worldCenter.x * scale_;
  offset_.y = svgCenter.y + worldCenter.y * scale_;  // Y is flipped in SVG
}

void SVGGenerator::addPoint(const Point2D& point, const std::string& color,
                            double radius, const std::string& label) {
  Point2D svg_point = worldToSVG(point);
  svg_ << "  <circle cx=\"" << svg_point.x << "\" cy=\"" << svg_point.y
       << "\" r=\"" << radius << "\" fill=\"" << color << "\"/>\n";

  if (!label.empty()) {
    addText(Point2D(point.x, point.y - 1.0), label, color, 10.0);
  }
}

void SVGGenerator::addLine(const Point2D& start, const Point2D& end,
                           const std::string& color, double strokeWidth,
                           const std::string& style) {
  Point2D start_svg = worldToSVG(start);
  Point2D end_svg = worldToSVG(end);

  svg_ << "  <line x1=\"" << start_svg.x << "\" y1=\"" << start_svg.y
       << "\" x2=\"" << end_svg.x << "\" y2=\"" << end_svg.y << "\" stroke=\""
       << color << "\" stroke-width=\"" << strokeWidth << "\"";

  if (!style.empty()) {
    svg_ << " " << style;
  }

  svg_ << "/>\n";
}

void SVGGenerator::addArc(const Point2D& center, double radius,
                          double startAngle, double endAngle,
                          bool anticlockwise, const std::string& color,
                          double strokeWidth) {
  double radius_svg = worldToSVG(radius);

  // Calculate start and end points
  Point2D start(center.x + radius * std::cos(startAngle),
                center.y + radius * std::sin(startAngle));
  Point2D end(center.x + radius * std::cos(endAngle),
              center.y + radius * std::sin(endAngle));

  Point2D start_svg = worldToSVG(start);
  Point2D end_svg = worldToSVG(end);

  // Calculate if this is a large arc
  double angleDiff = endAngle - startAngle;
  if (anticlockwise) angleDiff = -angleDiff;
  if (angleDiff < 0) angleDiff += 2 * M_PI;
  bool largeArc = angleDiff > M_PI;

  svg_ << "  <path d=\"M " << start_svg.x << "," << start_svg.y << " A "
       << radius_svg << "," << radius_svg << " 0 " << (largeArc ? "1" : "0")
       << "," << (anticlockwise ? "0" : "1") << " " << end_svg.x << ","
       << end_svg.y << "\" stroke=\"" << color << "\" stroke-width=\""
       << strokeWidth << "\" fill=\"none\"/>\n";
}

void SVGGenerator::addCircle(const Point2D& center, double radius,
                             const std::string& color, double strokeWidth) {
  Point2D center_svg = worldToSVG(center);
  double radius_svg = worldToSVG(radius);

  svg_ << "  <circle cx=\"" << center_svg.x << "\" cy=\"" << center_svg.y
       << "\" r=\"" << radius_svg << "\" stroke=\"" << color
       << "\" stroke-width=\"" << strokeWidth << "\" fill=\"none\"/>\n";
}

void SVGGenerator::addText(const Point2D& position, const std::string& text,
                           const std::string& color, double fontSize) {
  Point2D svg_pos = worldToSVG(position);
  svg_ << "  <text x=\"" << svg_pos.x << "\" y=\"" << svg_pos.y
       << "\" font-family=\"Arial, sans-serif\" font-size=\"" << fontSize
       << "\" fill=\"" << color << "\" text-anchor=\"middle\">" << text
       << "</text>\n";
}

std::string SVGGenerator::generate() const {
  std::stringstream result;
  result << svg_.str();
  result << "</svg>\n";
  return result.str();
}

bool SVGGenerator::saveToFile(const std::string& filename) const {
  std::ofstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  file << generate();
  return file.good();
}

Point2D SVGGenerator::worldToSVG(const Point2D& world) const {
  return Point2D(offset_.x + world.x * scale_,
                 offset_.y - world.y * scale_);  // Y is flipped in SVG
}

double SVGGenerator::worldToSVG(double worldDistance) const {
  return worldDistance * scale_;
}

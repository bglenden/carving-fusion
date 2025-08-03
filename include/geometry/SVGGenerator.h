/**
 * SVG generator for visual verification of shape drawing
 * Creates SVG files that can be manually verified and used as truth files
 */

#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "Leaf.h"
#include "Point2D.h"
#include "TriArc.h"

namespace ChipCarving {
namespace Geometry {

/**
 * Simple SVG builder for creating visual verification files
 */
class SVGGenerator {
   private:
    std::stringstream svg_;
    double width_;
    double height_;
    double scale_;
    Point2D offset_;

   public:
    /**
     * Create SVG generator with specified canvas size
     * @param width Canvas width in SVG units
     * @param height Canvas height in SVG units
     * @param scale Scale factor (SVG units per mm)
     */
    explicit SVGGenerator(double width = 400.0, double height = 400.0, double scale = 10.0);

    /**
     * Set the coordinate system to center and fit the given bounds
     * @param min Minimum point in world coordinates
     * @param max Maximum point in world coordinates
     * @param margin Margin around the bounds (in world units)
     */
    void setBounds(const Point2D& min, const Point2D& max, double margin = 2.0);

    /**
     * Add shape outline to SVG
     */
    void addLeaf(const Leaf& leaf, const std::string& color = "black", double strokeWidth = 1.0);
    void addTriArc(const TriArc& triArc, const std::string& color = "black",
                   double strokeWidth = 1.0);

    /**
     * Add debug markers (arc centers, vertices, etc.)
     */
    void addDebugMarkers(const Leaf& leaf);
    void addTriArcDebugMarkers(const TriArc& triArc);

    /**
     * Add a point marker
     */
    void addPoint(const Point2D& point, const std::string& color = "red", double radius = 2.0,
                  const std::string& label = "");

    /**
     * Add a line
     */
    void addLine(const Point2D& start, const Point2D& end, const std::string& color = "gray",
                 double strokeWidth = 1.0, const std::string& style = "");

    /**
     * Add an arc path
     */
    void addArc(const Point2D& center, double radius, double startAngle, double endAngle,
                bool anticlockwise = false, const std::string& color = "blue",
                double strokeWidth = 1.0);

    /**
     * Add a circle (stroke only, not filled)
     */
    void addCircle(const Point2D& center, double radius, const std::string& color = "blue",
                   double strokeWidth = 1.0);

    /**
     * Add text label
     */
    void addText(const Point2D& position, const std::string& text,
                 const std::string& color = "black", double fontSize = 12.0);

    /**
     * Generate the complete SVG string
     */
    std::string generate() const;

    /**
     * Save SVG to file
     */
    bool saveToFile(const std::string& filename) const;

   private:
    /**
     * Convert world coordinates to SVG coordinates
     */
    Point2D worldToSVG(const Point2D& world) const;

    /**
     * Convert world distance to SVG distance
     */
    double worldToSVG(double worldDistance) const;
};

/**
 * Utility class for comparing SVG files with numerical tolerance
 */
class SVGComparator {
   public:
    static constexpr double DEFAULT_TOLERANCE = 1e-6;

    /**
     * Compare two SVG files for geometric equivalence
     * @param file1 First SVG file path
     * @param file2 Second SVG file path
     * @param tolerance Numerical tolerance for coordinate comparison
     * @return true if files are geometrically equivalent within tolerance
     */
    static bool compare(const std::string& file1, const std::string& file2,
                        double tolerance = DEFAULT_TOLERANCE);

    /**
     * Extract numerical values from SVG path data
     */
    static std::vector<double> extractNumbers(const std::string& svgContent);

    /**
     * Compare two sets of numbers with tolerance
     */
    static bool compareNumbers(const std::vector<double>& numbers1,
                               const std::vector<double>& numbers2, double tolerance);
};

}  // namespace Geometry
}  // namespace ChipCarving
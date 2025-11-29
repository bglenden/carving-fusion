/**
 * Standalone medial axis test program
 * Tests complete pipeline: Shape -> Polygon -> OpenVoronoi -> Medial Axis -> SVG Output
 *
 * This program verifies the medial axis computation outside of Fusion 360,
 * generating visual outputs for verification and truth file comparison.
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// OpenVoronoi includes
#include <medial_axis_filter.hpp>
#include <medial_axis_walk.hpp>
#include <polygon_interior_filter.hpp>
#include <version.hpp>
#include <voronoidiagram.hpp>

// Our shape classes
#include "geometry/Leaf.h"
#include "geometry/MedialAxisUtilities.h"
#include "geometry/SVGGenerator.h"
#include "geometry/Shape.h"
#include "geometry/TriArc.h"

using namespace ChipCarving::Geometry;

/**
 * Coordinate transformation parameters for unit circle conversion
 */
struct TransformParams {
    Point2D offset;       // Translation to center at origin
    double scale;         // Uniform scale factor to fit in unit circle
    Point2D originalMin;  // Original bounding box minimum
    Point2D originalMax;  // Original bounding box maximum

    TransformParams() : offset(0, 0), scale(1.0), originalMin(0, 0), originalMax(0, 0) {}
};

/**
 * Medial axis computation results
 */
struct MedialAxisResults {
    std::vector<std::vector<Point2D>> chains;         // Medial axis chains in world coordinates
    std::vector<std::vector<double>> clearanceRadii;  // Clearance radii for each chain point
    TransformParams transform;                        // Transform parameters used

    // Statistics
    int numChains = 0;
    int totalPoints = 0;
    double totalLength = 0.0;
    double minClearance = 0.0;
    double maxClearance = 0.0;
};

/**
 * Transform polygon from world coordinates to unit circle
 */
std::vector<Point2D> transformToUnitCircle(const std::vector<Point2D>& polygon,
                                           TransformParams& params) {
    if (polygon.empty()) {
        return polygon;
    }

    // Calculate bounding box manually (calculateBounds removed)
    params.originalMin = params.originalMax = polygon[0];
    for (const auto& point : polygon) {
        if (point.x < params.originalMin.x) params.originalMin.x = point.x;
        if (point.y < params.originalMin.y) params.originalMin.y = point.y;
        if (point.x > params.originalMax.x) params.originalMax.x = point.x;
        if (point.y > params.originalMax.y) params.originalMax.y = point.y;
    }

    // Calculate center and size
    Point2D center = Point2D((params.originalMin.x + params.originalMax.x) * 0.5,
                             (params.originalMin.y + params.originalMax.y) * 0.5);
    double width = params.originalMax.x - params.originalMin.x;
    double height = params.originalMax.y - params.originalMin.y;
    double maxDimension = std::max(width, height);

    // Calculate transform parameters
    params.offset = Point2D(-center.x, -center.y);  // Translate to origin
    params.scale = 0.95 / maxDimension;             // Scale to fit in unit circle (with margin)

    // Apply transformation
    std::vector<Point2D> transformed;
    transformed.reserve(polygon.size());

    for (const auto& point : polygon) {
        Point2D translated = point + params.offset;
        Point2D scaled = Point2D(translated.x * params.scale, translated.y * params.scale);
        transformed.push_back(scaled);
    }

    return transformed;
}

/**
 * Transform results from unit circle back to world coordinates
 */
Point2D transformFromUnitCircle(const Point2D& unitPoint, const TransformParams& params) {
    // Reverse scaling then translation
    Point2D scaled = Point2D(unitPoint.x / params.scale, unitPoint.y / params.scale);
    Point2D worldPoint = scaled - params.offset;
    return worldPoint;
}

/**
 * Compute medial axis using OpenVoronoi
 */
MedialAxisResults computeMedialAxis(const std::vector<Point2D>& polygon, double threshold = 0.8) {
    MedialAxisResults results;

    if (polygon.size() < 3) {
        std::cerr << "Error: Polygon must have at least 3 vertices" << std::endl;
        return results;
    }

    // Transform to unit circle
    std::vector<Point2D> transformedPolygon = transformToUnitCircle(polygon, results.transform);

    std::cout << "Original bounds: (" << results.transform.originalMin.x << ", "
              << results.transform.originalMin.y << ") to (" << results.transform.originalMax.x
              << ", " << results.transform.originalMax.y << ")" << std::endl;
    std::cout << "Transform: offset=(" << results.transform.offset.x << ", "
              << results.transform.offset.y << "), scale=" << results.transform.scale << std::endl;

    // Create VoronoiDiagram
    int numSites = static_cast<int>(transformedPolygon.size());
    int bins = std::max(10, static_cast<int>(std::sqrt(numSites)));
    ovd::VoronoiDiagram* vd = new ovd::VoronoiDiagram(1.0, bins);

    std::cout << "OpenVoronoi version: " << ovd::version() << std::endl;
    std::cout << "Processing polygon with " << numSites << " vertices, using " << bins << " bins"
              << std::endl;

    // Insert point sites
    std::vector<int> pointIds;
    pointIds.reserve(numSites);

    for (const auto& point : transformedPolygon) {
        ovd::Point ovdPoint(point.x, point.y);
        int id = vd->insert_point_site(ovdPoint);
        pointIds.push_back(id);
        std::cout << "Added point " << id << ": (" << point.x << ", " << point.y << ")"
                  << std::endl;
    }

    // Insert line sites (connecting consecutive points)
    for (size_t i = 0; i < pointIds.size(); ++i) {
        int startId = pointIds[i];
        int endId = pointIds[(i + 1) % pointIds.size()];
        vd->insert_line_site(startId, endId);
        std::cout << "Added line site: " << startId << " -> " << endId << std::endl;
    }

    // Validate the diagram
    bool isValid = vd->check();
    if (!isValid) {
        std::cerr << "Warning: Voronoi diagram validation failed" << std::endl;
    }

    // Apply filters
    ovd::polygon_interior_filter interiorFilter(true);
    vd->filter(&interiorFilter);

    ovd::medial_axis_filter medialFilter(threshold);
    vd->filter(&medialFilter);

    // Extract medial axis with custom vertex spacing
    ovd::HEGraph& graph = vd->get_graph_reference();

    // First get the basic medial axis structure
    ovd::MedialAxisWalk walker(graph, 3);  // Minimal curved edge points
    ovd::MedialChainList chainList = walker.walk();

    // Now enhance linear edges with additional vertices
    // For each chain, subdivide long linear segments
    for (auto& chain : chainList) {
        for (auto& pointList : chain) {
            if (pointList.size() == 2) {  // Likely a linear edge with only endpoints
                auto p1 = pointList.front();
                auto p2 = pointList.back();

                // Calculate distance between endpoints
                double edgeLength = sqrt(pow(p2.p.x - p1.p.x, 2) + pow(p2.p.y - p1.p.y, 2));

                std::cout << "  Found 2-point edge: length=" << edgeLength << "mm from (" << p1.p.x
                          << "," << p1.p.y << ") to (" << p2.p.x << "," << p2.p.y << ")"
                          << std::endl;

                // If edge is long enough, subdivide it
                if (edgeLength > 1.0) {  // More than 1mm, add intermediate points
                    std::cout << "    Subdividing edge of length " << edgeLength << "mm"
                              << std::endl;
                    std::list<ovd::MedialPoint> newPoints;
                    newPoints.push_back(p1);  // Keep start point

                    int numSegments =
                        std::max(2, static_cast<int>(edgeLength / 0.8));  // Point every 0.8mm

                    // Add intermediate points
                    for (int i = 1; i < numSegments; ++i) {
                        double t = double(i) / double(numSegments);

                        // Linear interpolation of position
                        ovd::Point newPos(p1.p.x + t * (p2.p.x - p1.p.x),
                                          p1.p.y + t * (p2.p.y - p1.p.y));

                        // Linear interpolation of clearance radius
                        double newClearance =
                            p1.clearance_radius + t * (p2.clearance_radius - p1.clearance_radius);

                        newPoints.emplace_back(newPos, newClearance);
                    }

                    newPoints.push_back(p2);  // Keep end point
                    pointList = newPoints;    // Replace with subdivided edge
                }
            }
        }
    }

    std::cout << "Found " << chainList.size() << " medial axis chains" << std::endl;

    // Convert results back to world coordinates
    results.numChains = static_cast<int>(chainList.size());
    results.minClearance = std::numeric_limits<double>::max();
    results.maxClearance = 0.0;

    for (const auto& chain : chainList) {
        std::vector<Point2D> worldChain;
        std::vector<double> worldClearances;

        for (const auto& pointList : chain) {
            for (const auto& medialPoint : pointList) {
                // Convert point back to world coordinates
                Point2D unitPoint(medialPoint.p.x, medialPoint.p.y);
                Point2D worldPoint = transformFromUnitCircle(unitPoint, results.transform);
                worldChain.push_back(worldPoint);

                // Convert clearance radius back to world scale
                double worldClearance = medialPoint.clearance_radius / results.transform.scale;
                worldClearances.push_back(worldClearance);

                // Update statistics
                results.totalPoints++;
                results.minClearance = std::min(results.minClearance, worldClearance);
                results.maxClearance = std::max(results.maxClearance, worldClearance);

                // Debug output for first few points only
                if (results.totalPoints <= 3) {
                    std::cout << "Medial point: (" << worldPoint.x << ", " << worldPoint.y
                              << "), clearance: " << worldClearance << std::endl;
                }
            }
        }

        if (!worldChain.empty()) {
            results.chains.push_back(worldChain);
            results.clearanceRadii.push_back(worldClearances);

            // Calculate chain length
            double chainLength = 0.0;
            for (size_t i = 1; i < worldChain.size(); ++i) {
                chainLength += distance(worldChain[i - 1], worldChain[i]);
            }
            results.totalLength += chainLength;
        }
    }

    // Clean up
    delete vd;

    std::cout << "Medial axis computation complete:" << std::endl;
    std::cout << "  Chains: " << results.numChains << std::endl;
    std::cout << "  Total points: " << results.totalPoints << std::endl;
    std::cout << "  Total length: " << results.totalLength << std::endl;
    std::cout << "  Clearance range: [" << results.minClearance << ", " << results.maxClearance
              << "]" << std::endl;

    return results;
}

/**
 * Generate layered SVG output showing all stages of processing
 */
void generateLayeredSVG(const std::string& testName, const Shape& originalShape,
                        const std::vector<Point2D>& polygon, const MedialAxisResults& results,
                        double maxError, double threshold) {
    SVGGenerator svg;

    // Calculate bounds that include all geometry
    Point2D min = results.transform.originalMin;
    Point2D max = results.transform.originalMax;

    // Expand bounds slightly for medial axis chains
    for (const auto& chain : results.chains) {
        for (const auto& point : chain) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
        }
    }

    // Add some margin
    double margin = std::max(max.x - min.x, max.y - min.y) * 0.1;
    min.x -= margin;
    min.y -= margin;
    max.x += margin;
    max.y += margin;

    svg.setBounds(min, max, 1.2);

    // Layer 1: Original shape (curved edges) in blue
    if (auto* leaf = dynamic_cast<const Leaf*>(&originalShape)) {
        svg.addLeaf(*leaf, "blue", 2.0);
    } else if (auto* triArc = dynamic_cast<const TriArc*>(&originalShape)) {
        svg.addTriArc(*triArc, "blue", 2.0);
    }

    // Layer 2: Polygonized shape (line segments) in red dashed
    if (!polygon.empty()) {
        for (size_t i = 0; i < polygon.size(); ++i) {
            Point2D start = polygon[i];
            Point2D end = polygon[(i + 1) % polygon.size()];
            svg.addLine(start, end, "red", 1.5, "stroke-dasharray=\"5,5\"");  // Dashed line
        }

        // Add vertices as small points
        for (const auto& vertex : polygon) {
            svg.addPoint(vertex, "red", 1.0);
        }
    }

    // Layer 3: Medial axis lines with different colors per path
    std::vector<std::string> pathColors = {"green", "purple", "brown", "teal", "navy", "maroon"};

    for (size_t pathIdx = 0; pathIdx < results.chains.size(); ++pathIdx) {
        const auto& chain = results.chains[pathIdx];
        if (chain.size() < 2)
            continue;

        // Use different color for each path, cycling through available colors
        std::string pathColor = pathColors[pathIdx % pathColors.size()];
        std::string darkPathColor = pathColor;  // Use same color for endpoints

        // Draw the medial axis lines for this path
        for (size_t i = 1; i < chain.size(); ++i) {
            svg.addLine(chain[i - 1], chain[i], pathColor, 2.0);
        }

        // Mark chain endpoints with path-specific color
        svg.addPoint(chain.front(), darkPathColor, 3.0, "START");
        svg.addPoint(chain.back(), darkPathColor, 3.0, "END");
    }

    // Layer 4: Clearance circles (evenly distributed along medial axis)
    std::vector<Point2D> allCircleCenters;  // Track all circles to avoid duplicates

    // Use utility function to sample medial axis paths
    std::vector<SampledMedialPath> sampledPaths =
        sampleMedialAxisPaths(results.chains, results.clearanceRadii, 1.0);  // 1mm spacing

    // Draw clearance circles for each sampled path with path-specific colors
    for (size_t pathIdx = 0; pathIdx < sampledPaths.size(); ++pathIdx) {
        const auto& sampledPath = sampledPaths[pathIdx];

        // Use same color scheme as medial axis lines
        std::string circleColor = pathColors[pathIdx % pathColors.size()];

        std::cout << "Path " << pathIdx << " (" << circleColor << "): " << sampledPath.points.size()
                  << " sampled points, length=" << sampledPath.totalLength << "mm" << std::endl;

        // Draw circles at sampled positions
        for (size_t idx = 0; idx < sampledPath.points.size(); ++idx) {
            const auto& sampledPoint = sampledPath.points[idx];

            // Check if we already have a circle very close to this position
            bool isDuplicate = false;
            for (const auto& existingCenter : allCircleCenters) {
                if (distance(existingCenter, sampledPoint.position) < 0.1) {  // 0.1mm tolerance
                    isDuplicate = true;
                    break;
                }
            }

            if (!isDuplicate) {
                // Draw clearance circle outline using path-specific color
                svg.addCircle(sampledPoint.position, sampledPoint.clearanceRadius, circleColor,
                              1.0);

                // Add a small point at the center
                svg.addPoint(sampledPoint.position, circleColor, 2.0);

                // Add text label positioned to avoid overlap
                std::ostringstream clearanceStr;
                clearanceStr << std::fixed << std::setprecision(2) << sampledPoint.clearanceRadius;

                // Position text outside the circle, alternating above/below
                double textOffset = sampledPoint.clearanceRadius + 1.5;
                bool above = idx % 2 == 0;  // Alternate text position
                Point2D textPos =
                    Point2D(sampledPoint.position.x,
                            sampledPoint.position.y + (above ? -textOffset : textOffset));
                svg.addText(textPos, clearanceStr.str(), circleColor, 9.0);

                // Remember this circle center
                allCircleCenters.push_back(sampledPoint.position);
            }
        }
    }

    // Add title and information with proper formatting
    std::ostringstream titleStr;
    titleStr << testName << " (error=" << std::fixed << std::setprecision(2) << maxError
             << "mm, threshold=" << std::setprecision(1) << threshold << ")";
    svg.addText(Point2D(min.x, max.y + margin * 0.3), titleStr.str(), "black", 16.0);

    std::string info = "Chains: " + std::to_string(results.numChains) +
                       ", Points: " + std::to_string(results.totalPoints) +
                       ", Length: " + std::to_string(static_cast<int>(results.totalLength)) + "mm";
    svg.addText(Point2D(min.x, max.y + margin * 0.1), info, "black", 12.0);

    // Add compact horizontal legend
    double legendX = max.x - margin * 0.95;
    double legendY = min.y + margin * 0.2;
    double lineLength = 2.0;
    double itemSpacing = 0.6;  // Vertical spacing between legend items

    svg.addText(Point2D(legendX, legendY), "Legend:", "black", 12.0);

    // Original shape line
    double y1 = legendY - itemSpacing;
    svg.addLine(Point2D(legendX, y1), Point2D(legendX + lineLength, y1), "blue", 2.0);
    svg.addText(Point2D(legendX + lineLength + 0.3, y1), "Original", "black", 10.0);

    // Polygon line
    double y2 = legendY - 2 * itemSpacing;
    svg.addLine(Point2D(legendX, y2), Point2D(legendX + lineLength, y2), "red", 1.5,
                "stroke-dasharray=\"5,5\"");
    svg.addText(Point2D(legendX + lineLength + 0.3, y2), "Polygon", "black", 10.0);

    // Medial axis paths (show first path color as example)
    double y3 = legendY - 3 * itemSpacing;
    svg.addLine(Point2D(legendX, y3), Point2D(legendX + lineLength, y3), pathColors[0], 2.0);
    svg.addText(Point2D(legendX + lineLength + 0.3, y3), "Medial Paths", "black", 10.0);

    // Clearance circles (show first path color as example)
    double y4 = legendY - 4 * itemSpacing;
    svg.addCircle(Point2D(legendX + lineLength / 2, y4), 0.2, pathColors[0], 1.0);
    svg.addText(Point2D(legendX + lineLength + 0.3, y4), "Clearance", "black", 10.0);

    // Save to file
    std::string filename = "medial_axis_" + testName + ".svg";
    if (svg.saveToFile(filename)) {
        std::cout << "Generated SVG: " << filename << std::endl;
    } else {
        std::cerr << "Failed to generate SVG: " << filename << std::endl;
    }
}

/**
 * Test a single shape and generate outputs
 */
void testShape(const std::string& testName, std::unique_ptr<Shape> shape, double maxError = 0.25,
               double threshold = 0.8) {
    std::cout << "\n=== Testing " << testName << " ===" << std::endl;
    std::cout << "Polygonization maxError: " << maxError << "mm" << std::endl;
    std::cout << "Medial axis threshold: " << threshold << std::endl;

    // Get polygon approximation
    std::vector<Point2D> polygon = shape->getPolygonVertices(maxError);
    std::cout << "Polygonized to " << polygon.size() << " vertices" << std::endl;

    // Compute medial axis
    MedialAxisResults results = computeMedialAxis(polygon, threshold);

    // Generate layered SVG output
    generateLayeredSVG(testName, *shape, polygon, results, maxError, threshold);

    // TODO: Save numerical results to JSON
    // TODO: Compare against truth files

    std::cout << "Test " << testName << " completed successfully" << std::endl;
}

/**
 * Main test program
 */
int main() {
    std::cout << "Standalone Medial Axis Test Program" << std::endl;
    std::cout << "====================================" << std::endl;

    try {
        // Test Case 1: Simple horizontal leaf
        {
            Point2D focus1(0.0, 0.0);
            Point2D focus2(10.0, 0.0);
            auto leaf = std::make_unique<Leaf>(focus1, focus2, 6.5);
            testShape("leaf_horizontal", std::move(leaf));
        }

        // Test Case 2: Simple triangle
        {
            Point2D v1(0.0, 0.0);
            Point2D v2(10.0, 0.0);
            Point2D v3(5.0, 8.66);  // Approximately equilateral triangle
            std::array<double, 3> bulges = {-0.125, -0.125, -0.125};
            auto triangle = std::make_unique<TriArc>(v1, v2, v3, bulges);
            testShape("triangle_curved", std::move(triangle), 0.25,
                      0.6);  // Lower threshold for more branches
        }

        // Test Case 3: Leaf with different error tolerance
        {
            Point2D focus1(-5.0, 0.0);
            Point2D focus2(5.0, 0.0);
            auto leaf = std::make_unique<Leaf>(focus1, focus2, 8.0);
            testShape("leaf_fine_tolerance", std::move(leaf), 0.1);  // Finer tolerance
        }

        std::cout << "\nAll tests completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
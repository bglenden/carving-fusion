#include <fstream>
#include <iostream>
#include <medial_axis_filter.hpp>
#include <medial_axis_walk.hpp>
#include <polygon_interior_filter.hpp>
#include <vector>
#include <voronoidiagram.hpp>

struct Point2D {
    double x, y;
    Point2D(double x = 0, double y = 0) : x(x), y(y) {}
};

// Function to calculate polygon area (signed area indicates winding order)
double calculateSignedArea(const std::vector<Point2D>& polygon) {
    double area = 0.0;
    size_t n = polygon.size();

    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        area += (polygon[j].x - polygon[i].x) * (polygon[j].y + polygon[i].y);
    }

    return area / 2.0;
}

// Transform polygon to unit circle
std::vector<Point2D> transformToUnitCircle(const std::vector<Point2D>& polygon) {
    if (polygon.empty())
        return polygon;

    // Calculate bounding box
    Point2D minP = polygon[0], maxP = polygon[0];
    for (const auto& p : polygon) {
        minP.x = std::min(minP.x, p.x);
        minP.y = std::min(minP.y, p.y);
        maxP.x = std::max(maxP.x, p.x);
        maxP.y = std::max(maxP.y, p.y);
    }

    // Calculate center and size
    Point2D center((minP.x + maxP.x) * 0.5, (minP.y + maxP.y) * 0.5);
    double width = maxP.x - minP.x;
    double height = maxP.y - minP.y;
    double maxDimension = std::max(width, height);

    // Transform parameters
    Point2D offset(-center.x, -center.y);
    double scale = 0.95 / maxDimension;

    std::cout << "Transform: center(" << center.x << ", " << center.y << "), scale=" << scale
              << std::endl;

    // Apply transformation
    std::vector<Point2D> transformed;
    for (const auto& point : polygon) {
        Point2D translated(point.x + offset.x, point.y + offset.y);
        Point2D scaled(translated.x * scale, translated.y * scale);
        transformed.push_back(scaled);
    }

    return transformed;
}

void writeSVG(const std::vector<Point2D>& polygon, const std::string& filename) {
    std::ofstream svg(filename);
    if (!svg.is_open())
        return;

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"800\" viewBox=\"-1.2 "
           "-1.2 2.4 2.4\">\\n";
    svg << "  <title>Standalone Medial Axis Test</title>\\n";
    svg << "  <rect x=\"-1.2\" y=\"-1.2\" width=\"2.4\" height=\"2.4\" fill=\"white\"/>\\n";
    svg << "  <circle cx=\"0\" cy=\"0\" r=\"1\" fill=\"none\" stroke=\"lightgray\" "
           "stroke-width=\"0.01\"/>\\n";

    // Draw polygon
    svg << "  <path d=\"M ";
    for (size_t i = 0; i < polygon.size(); ++i) {
        if (i > 0)
            svg << " L ";
        svg << polygon[i].x << " " << polygon[i].y;
    }
    svg << " Z\" fill=\"none\" stroke=\"blue\" stroke-width=\"0.02\"/>\\n";

    // Draw vertices with labels
    for (size_t i = 0; i < polygon.size(); ++i) {
        svg << "  <circle cx=\"" << polygon[i].x << "\" cy=\"" << polygon[i].y
            << "\" r=\"0.03\" fill=\"red\"/>\\n";
        svg << "  <text x=\"" << polygon[i].x + 0.05 << "\" y=\"" << polygon[i].y - 0.05
            << "\" font-size=\"0.06\" fill=\"black\">" << i << "</text>\\n";
    }

    svg << "</svg>\\n";
    svg.close();
}

int main() {
    // Test polygon from Fusion log data
    std::vector<Point2D> polygon = {
        {1.985216, -2.539599}, {2.542346, -1.799321}, {3.036261, -1.015452}, {3.463569, -0.193374},
        {3.821334, 0.661265},  {4.107100, 1.542595},  {4.318902, 2.444562},  {4.591039, 1.544441},
        {4.936780, 0.669946},  {5.353749, -0.172914}, {5.839081, -0.978352}, {6.389444, -1.740833},
        {7.001056, -2.455121}, {6.171789, -2.295278}, {5.332133, -2.204653}, {4.487856, -2.183870},
        {3.644759, -2.233072}, {2.808631, -2.351920}};

    std::cout << "=== Standalone Medial Axis Test ===" << std::endl;
    std::cout << "Original polygon: " << polygon.size() << " vertices" << std::endl;

    // Check polygon orientation
    double signedArea = calculateSignedArea(polygon);
    bool isCounterClockwise = signedArea > 0;
    std::cout << "Signed area: " << signedArea << std::endl;
    std::cout << "Winding order: " << (isCounterClockwise ? "Counter-clockwise" : "Clockwise")
              << std::endl;

    // Transform to unit circle
    std::vector<Point2D> transformed = transformToUnitCircle(polygon);

    // Write SVG for visualization
    std::string svgPath = "standalone_polygon_test.svg";
    writeSVG(transformed, svgPath);
    std::cout << "Wrote polygon SVG to " << svgPath
              << std::endl;

    try {
        // Create VoronoiDiagram
        ovd::VoronoiDiagram* vd = new ovd::VoronoiDiagram(1.0, 10);

        // Insert point sites
        std::vector<int> pointIds;
        for (size_t i = 0; i < transformed.size(); ++i) {
            const auto& point = transformed[i];
            ovd::Point ovdPoint(point.x, point.y);
            int id = vd->insert_point_site(ovdPoint);
            pointIds.push_back(id);
            std::cout << "Added point " << id << ": (" << point.x << ", " << point.y << ")"
                      << std::endl;
        }

        // Insert line sites
        for (size_t i = 0; i < pointIds.size(); ++i) {
            int startId = pointIds[i];
            int endId = pointIds[(i + 1) % pointIds.size()];
            vd->insert_line_site(startId, endId);
            std::cout << "Added line " << i << ": " << startId << " -> " << endId << std::endl;
        }

        // Validate diagram
        bool isValid = vd->check();
        std::cout << "Voronoi diagram valid: " << (isValid ? "YES" : "NO") << std::endl;

        // Test both filter configurations
        std::cout << "\\n=== Testing interior filter = false (for CCW) ===" << std::endl;
        {
            ovd::polygon_interior_filter interiorFilter(false);  // For CCW polygons
            vd->filter(&interiorFilter);

            ovd::medial_axis_filter medialFilter(0.8);
            vd->filter(&medialFilter);

            ovd::HEGraph& graph = vd->get_graph_reference();
            ovd::MedialAxisWalk walker(graph, 3);
            ovd::MedialChainList chainList = walker.walk();

            std::cout << "Found " << chainList.size() << " medial axis chains" << std::endl;
            for (size_t i = 0; i < chainList.size() && i < 3; ++i) {
                const auto& chain = chainList[i];
                std::cout << "  Chain " << i << ": " << chain.size() << " point lists" << std::endl;
                for (size_t j = 0; j < chain.size() && j < 2; ++j) {
                    const auto& pointList = chain[j];
                    std::cout << "    List " << j << ": " << pointList.size() << " points"
                              << std::endl;
                    for (auto it = pointList.begin();
                         it != pointList.end() && std::distance(pointList.begin(), it) < 3; ++it) {
                        std::cout << "      Point: (" << it->p.x << ", " << it->p.y
                                  << "), clearance: " << it->clearance_radius << std::endl;
                    }
                }
            }
        }

        // Reset and test opposite filter
        delete vd;
        vd = new ovd::VoronoiDiagram(1.0, 10);

        // Re-insert all sites
        pointIds.clear();
        for (size_t i = 0; i < transformed.size(); ++i) {
            const auto& point = transformed[i];
            ovd::Point ovdPoint(point.x, point.y);
            int id = vd->insert_point_site(ovdPoint);
            pointIds.push_back(id);
        }

        for (size_t i = 0; i < pointIds.size(); ++i) {
            int startId = pointIds[i];
            int endId = pointIds[(i + 1) % pointIds.size()];
            vd->insert_line_site(startId, endId);
        }

        std::cout << "\\n=== Testing interior filter = true (for CW) ===" << std::endl;
        {
            ovd::polygon_interior_filter interiorFilter(true);  // For CW polygons
            vd->filter(&interiorFilter);

            ovd::medial_axis_filter medialFilter(0.8);
            vd->filter(&medialFilter);

            ovd::HEGraph& graph = vd->get_graph_reference();
            ovd::MedialAxisWalk walker(graph, 3);
            ovd::MedialChainList chainList = walker.walk();

            std::cout << "Found " << chainList.size() << " medial axis chains" << std::endl;
            for (size_t i = 0; i < chainList.size() && i < 3; ++i) {
                const auto& chain = chainList[i];
                std::cout << "  Chain " << i << ": " << chain.size() << " point lists" << std::endl;
                for (size_t j = 0; j < chain.size() && j < 2; ++j) {
                    const auto& pointList = chain[j];
                    std::cout << "    List " << j << ": " << pointList.size() << " points"
                              << std::endl;
                    for (auto it = pointList.begin();
                         it != pointList.end() && std::distance(pointList.begin(), it) < 3; ++it) {
                        std::cout << "      Point: (" << it->p.x << ", " << it->p.y
                                  << "), clearance: " << it->clearance_radius << std::endl;
                    }
                }
            }
        }

        delete vd;

    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\\nTest completed successfully!" << std::endl;
    return 0;
}
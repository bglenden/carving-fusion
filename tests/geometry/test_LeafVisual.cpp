/**
 * Visual verification tests for Leaf shape
 * Generates SVG files that can be manually verified and used as truth files
 */

#include <gtest/gtest.h>

#include <filesystem>

#include "geometry/Leaf.h"
#include "geometry/SVGGenerator.h"

using namespace ChipCarving::Geometry;

class LeafVisualTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Create output directories
        std::filesystem::create_directories("truth_data");
        std::filesystem::create_directories("generated");
    }

    /**
     * Generate SVG for a leaf and optionally compare with truth file
     */
    void generateAndTest(const std::string& testName, const Leaf& leaf,
                         bool withDebugMarkers = true) {
        SVGGenerator svg;

        // Set bounds to show the leaf nicely - use simple approximation
        auto vertices = leaf.getVertices();
        Point2D centroid = leaf.getCentroid();
        double size = 20.0; // Simple fixed size for visualization
        Point2D min(centroid.x - size, centroid.y - size);
        Point2D max(centroid.x + size, centroid.y + size);
        svg.setBounds(min, max, 2.0);

        // Add the leaf shape
        svg.addLeaf(leaf, "black", 2.0);

        if (withDebugMarkers) {
            svg.addDebugMarkers(leaf);
        }

        // Add test information
        svg.addText(Point2D(min.x, max.y + 1.0), testName, "black", 14.0);

        // Save generated SVG
        std::string generatedFile = "generated/" + testName + ".svg";
        ASSERT_TRUE(svg.saveToFile(generatedFile)) << "Failed to save " << generatedFile;

        // Check if truth file exists for comparison
        std::string truthFile = "truth_data/" + testName + ".svg";
        if (std::filesystem::exists(truthFile)) {
            EXPECT_TRUE(SVGComparator::compare(truthFile, generatedFile, 1e-3))
                << "Generated SVG differs from truth file for " << testName
                << "\nGenerated: " << generatedFile << "\nTruth: " << truthFile;
        } else {
            // Truth file doesn't exist - this is the first run
            std::cout << "Truth file " << truthFile << " doesn't exist. "
                      << "Generated " << generatedFile << " for manual verification." << std::endl;
        }
    }

    static constexpr double TOLERANCE = 1e-6;
};

TEST_F(LeafVisualTest, DefaultLeaf) {
    // Standard horizontal leaf with default radius
    Point2D focus1(0.0, 0.0);
    Point2D focus2(10.0, 0.0);
    Leaf leaf(focus1, focus2);  // Uses default radius = 6.5mm

    generateAndTest("leaf_default", leaf);

    // Verify expected properties
    EXPECT_NEAR(leaf.getRadius(), 6.5, TOLERANCE);
    EXPECT_TRUE(leaf.isValidGeometry());
}

TEST_F(LeafVisualTest, NearlyRoundLeaf) {
    // Small radius relative to chord = nearly round (fat)
    Point2D focus1(0.0, 0.0);
    Point2D focus2(10.0, 0.0);
    Leaf leaf(focus1, focus2, 5.1);  // Just above minimum for valid geometry

    generateAndTest("leaf_nearly_round", leaf);

    EXPECT_TRUE(leaf.isValidGeometry());
    EXPECT_GT(leaf.getSagitta(), 2.0);  // Should have large sagitta (fat)
}

TEST_F(LeafVisualTest, NearlyFlatLeaf) {
    // Large radius relative to chord = nearly flat (thin)
    Point2D focus1(0.0, 0.0);
    Point2D focus2(10.0, 0.0);
    Leaf leaf(focus1, focus2, 50.0);  // Much larger than chord

    generateAndTest("leaf_nearly_flat", leaf);

    EXPECT_TRUE(leaf.isValidGeometry());
    EXPECT_LT(leaf.getSagitta(), 0.5);  // Should have small sagitta (thin)
}

TEST_F(LeafVisualTest, VerticalLeaf) {
    // Vertical orientation
    Point2D focus1(0.0, 0.0);
    Point2D focus2(0.0, 10.0);
    Leaf leaf(focus1, focus2);  // Default radius

    generateAndTest("leaf_vertical", leaf);

    EXPECT_TRUE(leaf.isValidGeometry());
}

TEST_F(LeafVisualTest, DiagonalLeaf) {
    // 45-degree diagonal orientation
    Point2D focus1(0.0, 0.0);
    Point2D focus2(7.071, 7.071);  // ~10mm apart at 45 degrees
    Leaf leaf(focus1, focus2);

    generateAndTest("leaf_diagonal", leaf);

    EXPECT_TRUE(leaf.isValidGeometry());
}

TEST_F(LeafVisualTest, PointUpTriangleLeaf) {
    // Leaf oriented with "point" up (simulating TriArc point-up case)
    Point2D focus1(-5.0, 0.0);
    Point2D focus2(5.0, 0.0);
    Leaf leaf(focus1, focus2, 8.0);

    generateAndTest("leaf_point_up", leaf);

    EXPECT_TRUE(leaf.isValidGeometry());
}

TEST_F(LeafVisualTest, BaseUpTriangleLeaf) {
    // Leaf oriented with "base" up (simulating TriArc base-up case)
    Point2D focus1(-5.0, 8.66);  // Top of equilateral triangle
    Point2D focus2(5.0, 8.66);
    Leaf leaf(focus1, focus2, 8.0);

    generateAndTest("leaf_base_up", leaf);

    EXPECT_TRUE(leaf.isValidGeometry());
}

TEST_F(LeafVisualTest, InvalidGeometry) {
    // Invalid leaf (radius too small)
    Point2D focus1(0.0, 0.0);
    Point2D focus2(10.0, 0.0);
    Leaf leaf(focus1, focus2, 4.0);  // Less than half chord length

    generateAndTest("leaf_invalid", leaf);

    EXPECT_FALSE(leaf.isValidGeometry());
}

TEST_F(LeafVisualTest, EdgeCaseMinimumRadius) {
    // Edge case: radius exactly half chord length
    Point2D focus1(0.0, 0.0);
    Point2D focus2(10.0, 0.0);
    Leaf leaf(focus1, focus2, 5.0);  // Exactly half chord length

    generateAndTest("leaf_edge_minimum", leaf);

    EXPECT_TRUE(leaf.isValidGeometry());
    // For radius = half chord length, this is actually a full semicircle, not zero sagitta
    EXPECT_GT(leaf.getSagitta(), 4.0);  // Should be significant sagitta (semicircle)
}

TEST_F(LeafVisualTest, ComparisonSheet) {
    // Create a comparison sheet with multiple leaves showing different curvatures
    SVGGenerator svg(800, 600);

    // Set bounds for the comparison
    svg.setBounds(Point2D(-15.0, -8.0), Point2D(65.0, 12.0));

    // Create leaves with different radii to show curvature differences
    std::vector<std::pair<std::string, double>> testCases = {{"Nearly Round (r=5.1)", 5.1},
                                                             {"Default (r=6.5)", 6.5},
                                                             {"Medium (r=10.0)", 10.0},
                                                             {"Large (r=20.0)", 20.0},
                                                             {"Nearly Flat (r=50.0)", 50.0}};

    for (size_t i = 0; i < testCases.size(); ++i) {
        double x_offset = i * 12.0;
        Point2D f1(-5.0 + x_offset, 0.0);
        Point2D f2(5.0 + x_offset, 0.0);

        Leaf leaf(f1, f2, testCases[i].second);

        // Add leaf without debug markers for cleaner comparison
        svg.addLeaf(leaf, "black", 1.5);

        // Add focus points
        svg.addPoint(f1, "red", 1.5);
        svg.addPoint(f2, "red", 1.5);

        // Add label
        svg.addText(Point2D(x_offset, -5.0), testCases[i].first, "black", 10.0);

        // Add radius value
        svg.addText(Point2D(x_offset, -6.5),
                    "sagitta: " + std::to_string(leaf.getSagitta()).substr(0, 4), "blue", 8.0);
    }

    svg.addText(Point2D(25.0, 8.0), "Leaf Shape Curvature Comparison", "black", 16.0);

    ASSERT_TRUE(svg.saveToFile("generated/leaf_comparison.svg"));
}
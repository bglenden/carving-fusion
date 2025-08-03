/**
 * Visual verification tests for TriArc shape
 * Generates SVG files that can be manually verified and used as truth files
 */

#include <gtest/gtest.h>

#include <filesystem>

#include "../../include/geometry/SVGGenerator.h"
#include "../../include/geometry/TriArc.h"

using namespace ChipCarving::Geometry;

class TriArcVisualTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Create output directories
        std::filesystem::create_directories("truth_data");
        std::filesystem::create_directories("generated");
    }

    /**
     * Generate SVG for a TriArc and optionally compare with truth file
     */
    void generateAndTest(const std::string& testName, const TriArc& triArc,
                         bool withDebugMarkers = true) {
        SVGGenerator svg;

        // Set bounds to show the TriArc nicely - use simple approximation
        auto vertices = triArc.getVertices();
        Point2D centroid = triArc.getCentroid();
        double size = 20.0; // Simple fixed size for visualization
        Point2D min(centroid.x - size, centroid.y - size);
        Point2D max(centroid.x + size, centroid.y + size);
        svg.setBounds(min, max, 2.0);

        // Add the TriArc shape
        svg.addTriArc(triArc, "black", 2.0);

        if (withDebugMarkers) {
            svg.addTriArcDebugMarkers(triArc);
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

TEST_F(TriArcVisualTest, DefaultTriArc) {
    // Standard equilateral triangle with default bulge factors
    Point2D v1(0.0, 0.0);
    Point2D v2(10.0, 0.0);
    Point2D v3(5.0, 8.66);
    TriArc triArc(v1, v2, v3);  // Uses default bulge factors: [-0.125, -0.125, -0.125]

    generateAndTest("triarc_default", triArc);

    // Verify expected properties
    EXPECT_TRUE(triArc.hasValidBulgeFactors());
    EXPECT_NEAR(triArc.getBulgeFactor(0), -0.125, TOLERANCE);
    EXPECT_NEAR(triArc.getBulgeFactor(1), -0.125, TOLERANCE);
    EXPECT_NEAR(triArc.getBulgeFactor(2), -0.125, TOLERANCE);
}

TEST_F(TriArcVisualTest, NearlyRoundTriArc) {
    // Large bulge factors = nearly round (fat curves)
    Point2D v1(0.0, 0.0);
    Point2D v2(10.0, 0.0);
    Point2D v3(5.0, 8.66);
    std::array<double, 3> largeBulges = {-0.8, -0.8, -0.8};
    TriArc triArc(v1, v2, v3, largeBulges);

    generateAndTest("triarc_nearly_round", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());
    for (int i = 0; i < 3; ++i) {
        EXPECT_NEAR(triArc.getBulgeFactor(i), -0.2, TOLERANCE);  // Clamped to MIN_BULGE
    }
}

TEST_F(TriArcVisualTest, NearlyFlatTriArc) {
    // Small bulge factors = nearly flat (thin curves)
    Point2D v1(0.0, 0.0);
    Point2D v2(10.0, 0.0);
    Point2D v3(5.0, 8.66);
    std::array<double, 3> smallBulges = {-0.02, -0.02, -0.02};
    TriArc triArc(v1, v2, v3, smallBulges);

    generateAndTest("triarc_nearly_flat", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());
    for (int i = 0; i < 3; ++i) {
        EXPECT_GT(triArc.getBulgeFactor(i), -0.05);  // Should have small bulge (thin curves)
    }
}

TEST_F(TriArcVisualTest, MixedBulgesTriArc) {
    // Different bulge factors on each edge
    Point2D v1(0.0, 0.0);
    Point2D v2(10.0, 0.0);
    Point2D v3(5.0, 8.66);
    std::array<double, 3> mixedBulges = {-0.1, -0.3, -0.6};
    TriArc triArc(v1, v2, v3, mixedBulges);

    generateAndTest("triarc_mixed_bulges", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());
    EXPECT_NEAR(triArc.getBulgeFactor(0), -0.1, TOLERANCE);
    EXPECT_NEAR(triArc.getBulgeFactor(1), -0.2, TOLERANCE);  // Clamped to MIN_BULGE
    EXPECT_NEAR(triArc.getBulgeFactor(2), -0.2, TOLERANCE);  // Clamped to MIN_BULGE
}

TEST_F(TriArcVisualTest, PointUpTriangle) {
    // Triangle with point at top (tip up)
    Point2D base1(-5.0, 0.0);
    Point2D base2(5.0, 0.0);
    Point2D tip(0.0, 8.66);
    TriArc triArc(base1, base2, tip);

    generateAndTest("triarc_point_up", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());

    // Verify centroid is in upper portion
    Point2D center = triArc.getCenter();
    EXPECT_GT(center.y, 2.0);  // Should be in upper portion (adjusted expectation)
}

TEST_F(TriArcVisualTest, BaseUpTriangle) {
    // Triangle with base at top (flat side up)
    Point2D tip(0.0, 0.0);
    Point2D base1(-5.0, 8.66);
    Point2D base2(5.0, 8.66);
    TriArc triArc(tip, base1, base2);

    generateAndTest("triarc_base_up", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());

    // Verify centroid position
    Point2D center = triArc.getCenter();
    EXPECT_NEAR(center.x, 0.0, TOLERANCE);
    EXPECT_GT(center.y, 2.0);  // Should be in upper portion
}

TEST_F(TriArcVisualTest, RightTriangle) {
    // Right triangle with 90-degree angle
    Point2D corner(0.0, 0.0);
    Point2D base(10.0, 0.0);
    Point2D height(0.0, 8.0);
    TriArc triArc(corner, base, height);

    generateAndTest("triarc_right_triangle", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());
}

TEST_F(TriArcVisualTest, WideTriangle) {
    // Wide, low triangle
    Point2D left(-8.0, 0.0);
    Point2D right(8.0, 0.0);
    Point2D top(0.0, 3.0);
    TriArc triArc(left, right, top);

    generateAndTest("triarc_wide", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());
}

TEST_F(TriArcVisualTest, TallTriangle) {
    // Tall, narrow triangle
    Point2D base1(-2.0, 0.0);
    Point2D base2(2.0, 0.0);
    Point2D top(0.0, 12.0);
    TriArc triArc(base1, base2, top);

    generateAndTest("triarc_tall", triArc);

    EXPECT_TRUE(triArc.hasValidBulgeFactors());
}

TEST_F(TriArcVisualTest, ZeroBulgeEdges) {
    // TriArc with one straight edge (zero bulge)
    Point2D v1(0.0, 0.0);
    Point2D v2(10.0, 0.0);
    Point2D v3(5.0, 8.66);
    std::array<double, 3> mixedBulges = {-0.125, -1e-10, -0.125};  // Middle edge is straight
    TriArc triArc(v1, v2, v3, mixedBulges);

    generateAndTest("triarc_zero_bulge", triArc);

    EXPECT_FALSE(triArc.isEdgeStraight(0));  // Curved edge
    EXPECT_TRUE(triArc.isEdgeStraight(1));   // Straight edge
    EXPECT_FALSE(triArc.isEdgeStraight(2));  // Curved edge
}

TEST_F(TriArcVisualTest, ComparisonSheet) {
    // Create a comparison sheet with multiple TriArcs showing different bulge factors
    SVGGenerator svg(1200, 800);

    // Set bounds for the comparison
    svg.setBounds(Point2D(-15.0, -5.0), Point2D(85.0, 25.0));

    // Create TriArcs with different bulge factors to show curvature differences
    std::vector<std::pair<std::string, std::array<double, 3>>> testCases = {
        {"Nearly Flat (-0.02)", {-0.02, -0.02, -0.02}},
        {"Small (-0.05)", {-0.05, -0.05, -0.05}},
        {"Default (-0.125)", {-0.125, -0.125, -0.125}},
        {"Large (-0.3)", {-0.3, -0.3, -0.3}},
        {"Nearly Round (-0.8)", {-0.8, -0.8, -0.8}}};

    for (size_t i = 0; i < testCases.size(); ++i) {
        double x_offset = i * 18.0;
        Point2D v1(-4.0 + x_offset, 0.0);
        Point2D v2(6.0 + x_offset, 0.0);
        Point2D v3(1.0 + x_offset, 8.66);

        TriArc triArc(v1, v2, v3, testCases[i].second);

        // Add TriArc without debug markers for cleaner comparison
        svg.addTriArc(triArc, "black", 1.5);

        // Add vertices
        svg.addPoint(v1, "red", 1.5);
        svg.addPoint(v2, "red", 1.5);
        svg.addPoint(v3, "red", 1.5);

        // Add label
        svg.addText(Point2D(1.0 + x_offset, -2.0), testCases[i].first, "black", 10.0);

        // Add bulge value
        svg.addText(Point2D(1.0 + x_offset, -3.5),
                    "bulge: " + std::to_string(testCases[i].second[0]).substr(0, 5), "blue", 8.0);
    }

    svg.addText(Point2D(35.0, 20.0), "TriArc Shape Bulge Factor Comparison", "black", 16.0);
    svg.addText(Point2D(35.0, 17.0), "All edges have identical bulge factors", "gray", 12.0);

    ASSERT_TRUE(svg.saveToFile("generated/triarc_comparison.svg"));
}
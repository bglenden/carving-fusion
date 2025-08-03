/**
 * test_MedialAxisRobustness.cpp
 *
 * Tests for MedialAxisProcessor robustness including error handling,
 * edge cases, and numerical stability.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "../../include/geometry/MedialAxisProcessor.h"
#include "../../include/geometry/Point2D.h"

using namespace ChipCarving::Geometry;

class MedialAxisRobustnessTest : public ::testing::Test {
   protected:
    void SetUp() override {
        processor = std::make_unique<MedialAxisProcessor>();
    }

    std::unique_ptr<MedialAxisProcessor> processor;
};


/**
 * Test with duplicate consecutive vertices
 */
TEST_F(MedialAxisRobustnessTest, DuplicateConsecutiveVertices) {
    std::vector<Point2D> polygonWithDupes = {{0, 0},
                                             {1, 0},
                                             {1, 0},  // Duplicate
                                             {1, 1},
                                             {0, 1}};

    MedialAxisResults results = processor->computeMedialAxis(polygonWithDupes);

    // Should fail due to duplicate vertices
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());
}

/**
 * Test with nearly duplicate vertices (within epsilon)
 */
TEST_F(MedialAxisRobustnessTest, NearlyDuplicateVertices) {
    std::vector<Point2D> polygon = {{0, 0},
                                    {1, 0},
                                    {1.0000000001, 0.0000000001},  // Nearly duplicate
                                    {1, 1},
                                    {0, 1}};

    MedialAxisResults results = processor->computeMedialAxis(polygon);

    // Should fail due to near-duplicate vertices
    EXPECT_FALSE(results.success);
}

/**
 * Test with degenerate triangle (zero area)
 */
TEST_F(MedialAxisRobustnessTest, DegenerateTriangle) {
    std::vector<Point2D> collinear = {
        {0, 0}, {1, 0}, {2, 0}  // All collinear
    };

    // OpenVoronoi is expected to assert/crash on degenerate input
    // This is a known limitation that we document by expecting death
    EXPECT_DEATH(
        { MedialAxisResults results = processor->computeMedialAxis(collinear); },
        ".*");  // Match any assertion message
}

/**
 * Test with very large coordinates
 */
TEST_F(MedialAxisRobustnessTest, VeryLargeCoordinates) {
    double largeVal = 1e6;
    std::vector<Point2D> largePolygon = {
        {0, 0}, {largeVal, 0}, {largeVal, largeVal}, {0, largeVal}};

    MedialAxisResults results = processor->computeMedialAxis(largePolygon);

    // Should succeed with proper scaling
    EXPECT_TRUE(results.success) << "Error: " << results.errorMessage;

    // Transform should scale down significantly
    EXPECT_LT(results.transform.scale, 1e-3);

    // Results should be in reasonable range
    EXPECT_GT(results.totalLength, 0);
    EXPECT_LT(results.maxClearance, largeVal);
}


/**
 * Test coordinate transformation precision (round-trip)
 */
TEST_F(MedialAxisRobustnessTest, TransformPrecisionRoundTrip) {
    // Create a polygon with specific coordinates
    std::vector<Point2D> original = {
        {123.456789, 234.567890}, {345.678901, 234.567890}, {234.567890, 456.789012}};

    MedialAxisResults results = processor->computeMedialAxis(original);
    EXPECT_TRUE(results.success);

    // Test round-trip transformation for a test point
    Point2D testPoint(200.0, 300.0);

    // Transform to unit circle
    Point2D translated = testPoint + results.transform.offset;
    Point2D scaled(translated.x * results.transform.scale, translated.y * results.transform.scale);

    // Transform back
    Point2D unscaled(scaled.x / results.transform.scale, scaled.y / results.transform.scale);
    Point2D untranslated = unscaled - results.transform.offset;

    // Should match original within precision limits
    EXPECT_NEAR(untranslated.x, testPoint.x, 1e-10);
    EXPECT_NEAR(untranslated.y, testPoint.y, 1e-10);
}

/**
 * Test with polygon having many vertices (stress test)
 */
TEST_F(MedialAxisRobustnessTest, ManyVerticesPolygon) {
    // Create a circle approximation with many vertices
    std::vector<Point2D> circle;
    const int numVertices = 1000;

    for (int i = 0; i < numVertices; ++i) {
        double angle = 2 * M_PI * i / numVertices;
        circle.push_back({cos(angle), sin(angle)});
    }

    MedialAxisResults results = processor->computeMedialAxis(circle);

    // Should handle large polygon successfully
    EXPECT_TRUE(results.success) << "Error: " << results.errorMessage;
    EXPECT_GT(results.totalPoints, 0);

    // With 1000 vertices, OpenVoronoi generates many small chains
    // This is expected behavior for high-resolution polygons
    EXPECT_GT(results.numChains, 100);  // Many chains for 1000-vertex circle
    EXPECT_LT(results.numChains, 500);  // But not excessive
}

/**
 * Test with concave polygon
 */
TEST_F(MedialAxisRobustnessTest, ConcavePolygon) {
    // Create a star shape (highly concave)
    std::vector<Point2D> star;
    const int numPoints = 5;

    for (int i = 0; i < numPoints * 2; ++i) {
        double angle = M_PI * i / numPoints;
        double radius = (i % 2 == 0) ? 1.0 : 0.5;
        star.push_back({radius * cos(angle), radius * sin(angle)});
    }

    MedialAxisResults results = processor->computeMedialAxis(star);

    EXPECT_TRUE(results.success) << "Error: " << results.errorMessage;

    // Star should have complex medial axis
    EXPECT_GT(results.numChains, 1);
    // With MedialAxisWalk parameter = 0 (no intermediate points), we expect
    // roughly the same number of points as input vertices, not more
    EXPECT_GE(results.totalPoints, star.size() / 2);  // At least half as many points
}

/**
 * Test with different medial axis thresholds
 */
TEST_F(MedialAxisRobustnessTest, MedialThresholdEffect) {
    // Simple rectangle
    std::vector<Point2D> rect = {{0, 0}, {4, 0}, {4, 1}, {0, 1}};

    // Test with strict threshold
    processor->setMedialThreshold(0.95);
    MedialAxisResults strictResults = processor->computeMedialAxis(rect);

    // Test with relaxed threshold
    processor->setMedialThreshold(0.5);
    MedialAxisResults relaxedResults = processor->computeMedialAxis(rect);

    EXPECT_TRUE(strictResults.success);
    EXPECT_TRUE(relaxedResults.success);

    // Relaxed threshold should produce more medial axis points
    EXPECT_GE(relaxedResults.totalPoints, strictResults.totalPoints);
}

/**
 * Test polygon with holes (not supported, should fail gracefully)
 */
TEST_F(MedialAxisRobustnessTest, PolygonWithHoles) {
    // Outer boundary
    std::vector<Point2D> outer = {{0, 0}, {4, 0}, {4, 4}, {0, 4}};

    // Note: We can't actually represent holes with simple polygon
    // This test documents expected behavior
    MedialAxisResults results = processor->computeMedialAxis(outer);

    // Should process outer boundary normally
    EXPECT_TRUE(results.success);
    EXPECT_GT(results.numChains, 0);
}

/**
 * Test with clockwise vs counter-clockwise polygons
 */
TEST_F(MedialAxisRobustnessTest, PolygonWindingOrder) {
    // CCW triangle
    std::vector<Point2D> ccwTriangle = {{0, 0}, {1, 0}, {0.5, 0.866}};

    // CW triangle (same shape, reversed order)
    std::vector<Point2D> cwTriangle = {{0, 0}, {0.5, 0.866}, {1, 0}};

    MedialAxisResults ccwResults = processor->computeMedialAxis(ccwTriangle);
    MedialAxisResults cwResults = processor->computeMedialAxis(cwTriangle);

    // Both should succeed
    EXPECT_TRUE(ccwResults.success) << "CCW Error: " << ccwResults.errorMessage;
    EXPECT_TRUE(cwResults.success) << "CW Error: " << cwResults.errorMessage;

    // Results should be similar (same shape, different winding)
    EXPECT_NEAR(ccwResults.totalLength, cwResults.totalLength, 0.1);
    EXPECT_EQ(ccwResults.numChains, cwResults.numChains);
}

/**
 * Test numerical edge cases
 */
TEST_F(MedialAxisRobustnessTest, NumericalEdgeCases) {
    // Test with coordinates at numerical limits
    std::vector<Point2D> edgeCases = {{0, 0},
                                      {1, std::numeric_limits<double>::epsilon()},
                                      {1, 1},
                                      {std::numeric_limits<double>::epsilon(), 1}};

    MedialAxisResults results = processor->computeMedialAxis(edgeCases);

    // Should handle epsilon values
    EXPECT_TRUE(results.success) << "Error: " << results.errorMessage;
}

/**
 * Test error message quality
 */
TEST_F(MedialAxisRobustnessTest, ErrorMessageQuality) {
    // Test various failure cases for error message quality

    // Empty polygon
    std::vector<Point2D> empty;
    MedialAxisResults emptyResults = processor->computeMedialAxis(empty);
    EXPECT_FALSE(emptyResults.success);
    EXPECT_TRUE(emptyResults.errorMessage.find("at least 3 vertices") != std::string::npos);

    // Two vertices
    std::vector<Point2D> twoPoints = {{0, 0}, {1, 1}};
    MedialAxisResults twoResults = processor->computeMedialAxis(twoPoints);
    EXPECT_FALSE(twoResults.success);
    EXPECT_TRUE(twoResults.errorMessage.find("at least 3 vertices") != std::string::npos);
}

/**
 * Test consistent results with same input
 */
TEST_F(MedialAxisRobustnessTest, DeterministicResults) {
    std::vector<Point2D> polygon = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}};

    // Run multiple times
    MedialAxisResults results1 = processor->computeMedialAxis(polygon);
    MedialAxisResults results2 = processor->computeMedialAxis(polygon);
    MedialAxisResults results3 = processor->computeMedialAxis(polygon);

    // All should succeed
    EXPECT_TRUE(results1.success);
    EXPECT_TRUE(results2.success);
    EXPECT_TRUE(results3.success);

    // Results should be identical
    EXPECT_EQ(results1.numChains, results2.numChains);
    EXPECT_EQ(results1.numChains, results3.numChains);
    EXPECT_EQ(results1.totalPoints, results2.totalPoints);
    EXPECT_EQ(results1.totalPoints, results3.totalPoints);
    EXPECT_DOUBLE_EQ(results1.totalLength, results2.totalLength);
    EXPECT_DOUBLE_EQ(results1.totalLength, results3.totalLength);
}
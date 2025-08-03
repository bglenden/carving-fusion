/**
 * test_MedialAxisProcessor.cpp
 *
 * Unit tests for MedialAxisProcessor class.
 * Tests medial axis computation, coordinate transformations, and error handling.
 *
 * NOTE: Shape-based medial axis tests have been removed because MedialAxisProcessor
 * now only supports polygon-based computation from Fusion profiles.
 * The deprecated shape-based computeMedialAxis(const Shape&) method is no longer supported.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <vector>

#include "../../include/geometry/Leaf.h"
#include "../../include/geometry/MedialAxisProcessor.h"
#include "../../include/geometry/TriArc.h"

using namespace ChipCarving::Geometry;

class MedialAxisProcessorTest : public ::testing::Test {
   protected:
    void SetUp() override {
        processor = std::make_unique<MedialAxisProcessor>();
        // Enable verbose logging for debugging if needed
        // processor->setVerbose(true);
    }

    void TearDown() override {
        processor.reset();
    }

    std::unique_ptr<MedialAxisProcessor> processor;
};

/**
 * Test basic processor construction and parameter access
 */
TEST_F(MedialAxisProcessorTest, ConstructorAndParameters) {
    // Default constructor
    MedialAxisProcessor defaultProcessor;
    EXPECT_EQ(defaultProcessor.getPolygonTolerance(), 0.25);
    EXPECT_EQ(defaultProcessor.getMedialThreshold(), 0.8);

    // Parameterized constructor
    MedialAxisProcessor customProcessor(0.1, 0.6);
    EXPECT_EQ(customProcessor.getPolygonTolerance(), 0.1);
    EXPECT_EQ(customProcessor.getMedialThreshold(), 0.6);

    // Parameter setters
    processor->setPolygonTolerance(0.5);
    processor->setMedialThreshold(0.9);
    EXPECT_EQ(processor->getPolygonTolerance(), 0.5);
    EXPECT_EQ(processor->getMedialThreshold(), 0.9);
}

/**
 * Test error handling for invalid polygons
 */
TEST_F(MedialAxisProcessorTest, InvalidPolygonHandling) {
    // Empty polygon
    std::vector<Point2D> emptyPolygon;
    MedialAxisResults results = processor->computeMedialAxis(emptyPolygon);
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());

    // Polygon with too few vertices
    std::vector<Point2D> twoVertices = {Point2D(0, 0), Point2D(1, 0)};
    results = processor->computeMedialAxis(twoVertices);
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());
}

/**
 * Test coordinate transformation to unit circle
 */
TEST_F(MedialAxisProcessorTest, CoordinateTransformation) {
    // Create a simple rectangle far from origin with large size
    std::vector<Point2D> rectangle = {Point2D(100, 100), Point2D(200, 100), Point2D(200, 150), Point2D(100, 150)};

    MedialAxisResults results = processor->computeMedialAxis(rectangle);

    // Should succeed with proper transformation
    EXPECT_TRUE(results.success) << "Error: " << results.errorMessage;

    // Check that transformation parameters are reasonable
    EXPECT_GT(results.transform.scale, 0.0);
    EXPECT_LT(results.transform.scale, 1.0);  // Should scale down large rectangle

    // Original bounds should match input
    EXPECT_EQ(results.transform.originalMin.x, 100);
    EXPECT_EQ(results.transform.originalMin.y, 100);
    EXPECT_EQ(results.transform.originalMax.x, 200);
    EXPECT_EQ(results.transform.originalMax.y, 150);
}

/**
 * Test medial axis computation for simple horizontal leaf
 * DEPRECATED: These tests use the deprecated shape-based medial axis computation.
 * The MedialAxisProcessor now only supports polygon-based computation from Fusion profiles.
 * Keeping these tests commented out for reference.
 */
/**
 * Test medial axis computation for triangular shape
 */

/**
 * Test different polygon tolerances
 */

/**
 * Test different medial axis thresholds
 */

/**
 * Test sampled path generation
 */

/**
 * Test sampled paths from failed computation
 */
TEST_F(MedialAxisProcessorTest, SampledPathsFromFailedComputation) {
    // Create failed results
    std::vector<Point2D> invalidPolygon = {Point2D(0, 0)};  // Too few vertices
    MedialAxisResults failedResults = processor->computeMedialAxis(invalidPolygon);
    EXPECT_FALSE(failedResults.success);

    // Should return empty paths
    std::vector<SampledMedialPath> paths = processor->getSampledPaths(failedResults, 1.0);
    EXPECT_TRUE(paths.empty());
}

/**
 * Test results structure consistency
 */

/**
 * Test with extremely small shape (edge case)
 */
TEST_F(MedialAxisProcessorTest, VerySmallShape) {
    Point2D focus1(0.0, 0.0);
    Point2D focus2(0.1, 0.0);  // Very small leaf
    Leaf smallLeaf(focus1, focus2, 0.08);

    MedialAxisResults results = processor->computeMedialAxis(smallLeaf);

    // Should either succeed or fail gracefully
    if (results.success) {
        EXPECT_GE(results.totalPoints, 0);
        EXPECT_GE(results.totalLength, 0.0);
    } else {
        EXPECT_FALSE(results.errorMessage.empty());
    }
}

/**
 * Test MedialAxisWalk parameter effect on point generation
 * Validates that the parameter change from 3 to 0 reduces intermediate points
 */
TEST_F(MedialAxisProcessorTest, MedialAxisWalkParameterEffect) {
    // Create a test polygon with enough complexity to show clear differences
    std::vector<Point2D> testPolygon = {
        Point2D(0.0, 0.0),    Point2D(5.0, 0.0),    Point2D(6.0, 2.0),    Point2D(5.0, 4.0),
        Point2D(3.0, 5.0),    Point2D(1.0, 4.0),    Point2D(-1.0, 3.0),   Point2D(-2.0, 1.0),
        Point2D(-1.0, -1.0),  Point2D(0.0, -2.0)
    };

    // Test with parameter = 3 (more intermediate points)
    processor->setMedialAxisWalkPoints(3);
    MedialAxisResults results3 = processor->computeMedialAxis(testPolygon);

    // Test with parameter = 0 (no intermediate points)
    processor->setMedialAxisWalkPoints(0);
    MedialAxisResults results0 = processor->computeMedialAxis(testPolygon);

    // Both should succeed
    EXPECT_TRUE(results3.success) << "Error with parameter=3: " << results3.errorMessage;
    EXPECT_TRUE(results0.success) << "Error with parameter=0: " << results0.errorMessage;

    // Parameter=0 should generate fewer points than parameter=3
    // Allow some tolerance but expect at least 15% reduction (more realistic)
    EXPECT_LT(results0.totalPoints, results3.totalPoints * 0.85)
        << "Parameter=0 points: " << results0.totalPoints 
        << ", Parameter=3 points: " << results3.totalPoints
        << " (expected significant reduction)";

    // Both should have same number of chains (topology unchanged)
    EXPECT_EQ(results0.numChains, results3.numChains);

    // Verify accessor works correctly
    EXPECT_EQ(processor->getMedialAxisWalkPoints(), 0);
    processor->setMedialAxisWalkPoints(5);
    EXPECT_EQ(processor->getMedialAxisWalkPoints(), 5);
}

/**
 * Test coordinate unit consistency (cm in results, mm in sampled paths)
 */
TEST_F(MedialAxisProcessorTest, CoordinateUnitConsistency) {
    // Create test polygon with known world coordinates (in cm as expected by MedialAxisProcessor)
    std::vector<Point2D> testPolygon = {
        Point2D(1.0, 1.0),   // 10mm, 10mm
        Point2D(4.0, 1.0),   // 40mm, 10mm  
        Point2D(4.0, 3.0),   // 40mm, 30mm
        Point2D(1.0, 3.0)    // 10mm, 30mm
    };

    // Compute medial axis
    MedialAxisResults results = processor->computeMedialAxis(testPolygon);
    EXPECT_TRUE(results.success) << "Error: " << results.errorMessage;

    // Verify MedialAxisResults stores coordinates in cm (same as input)
    bool foundPointInExpectedRange = false;
    for (const auto& chain : results.chains) {
        for (const auto& point : chain) {
            // Points should be in cm range (1-4 cm), not mm range (10-40 mm)
            if (point.x >= 1.0 && point.x <= 4.0 && point.y >= 1.0 && point.y <= 3.0) {
                foundPointInExpectedRange = true;
            }
        }
    }
    EXPECT_TRUE(foundPointInExpectedRange) << "No medial axis points found in expected cm coordinate range";

    // Test getSampledPaths converts cm -> mm (10x factor)
    auto sampledPaths = processor->getSampledPaths(results, 1.0);
    if (!sampledPaths.empty() && !sampledPaths[0].points.empty()) {
        bool foundSampledPointInMmRange = false;
        for (const auto& path : sampledPaths) {
            for (const auto& point : path.points) {
                // Sampled points should be in mm range (10-40 mm)
                if (point.position.x >= 8.0 && point.position.x <= 42.0 && point.position.y >= 8.0 && point.position.y <= 32.0) {
                    foundSampledPointInMmRange = true;
                }
            }
        }
        EXPECT_TRUE(foundSampledPointInMmRange) << "No sampled points found in expected mm coordinate range";
    }
}

/**
 * Test boundary point preservation (zero clearance at sharp corners)
 */
TEST_F(MedialAxisProcessorTest, PreservesBoundaryPoints) {
    // Create sharp triangle that should generate boundary points with zero clearance
    std::vector<Point2D> triangle = {
        Point2D(0.0, 0.0),   // Sharp corner
        Point2D(5.0, 0.0),   // Sharp corner  
        Point2D(2.5, 4.0)    // Sharp corner
    };

    MedialAxisResults results = processor->computeMedialAxis(triangle);
    EXPECT_TRUE(results.success) << "Error: " << results.errorMessage;

    // Count points with very small clearance (boundary points)
    int boundaryPointCount = 0;
    double boundaryTolerance = 0.01;  // 0.01 cm = 0.1mm tolerance

    for (const auto& clearanceChain : results.clearanceRadii) {
        for (double clearance : clearanceChain) {
            if (clearance < boundaryTolerance) {
                boundaryPointCount++;
            }
        }
    }

    // Should have at least one boundary point (triangle corners create zero-clearance points)
    EXPECT_GT(boundaryPointCount, 0) << "No boundary points with zero clearance found";

    // Verify boundary points exist in each chain
    for (size_t i = 0; i < results.clearanceRadii.size(); ++i) {
        bool chainHasBoundaryPoint = false;
        for (double clearance : results.clearanceRadii[i]) {
            if (clearance < boundaryTolerance) {
                chainHasBoundaryPoint = true;
                break;
            }
        }
        // At least some chains should have boundary points
        if (results.clearanceRadii[i].size() > 2) {  // Only check substantial chains
            EXPECT_TRUE(chainHasBoundaryPoint) 
                << "Chain " << i << " has no boundary points (may indicate missing sharp corners)";
        }
    }
}

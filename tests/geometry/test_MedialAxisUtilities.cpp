/**
 * test_MedialAxisUtilities.cpp
 *
 * Unit tests for medial axis sampling utilities
 */

#include <gtest/gtest.h>

#include <cmath>

#include "geometry/MedialAxisUtilities.h"

using namespace ChipCarving::Geometry;

class MedialAxisUtilitiesTest : public ::testing::Test {
   protected:
    // Helper to check if two points are approximately equal
    bool pointsEqual(const Point2D& a, const Point2D& b, double tolerance = 0.001) {
        return std::abs(a.x - b.x) < tolerance && std::abs(a.y - b.y) < tolerance;
    }

    // Helper to calculate path length
    double calculatePathLength(const std::vector<Point2D>& points) {
        double length = 0.0;
        for (size_t i = 1; i < points.size(); ++i) {
            length += distance(points[i - 1], points[i]);
        }
        return length;
    }
};

// Test empty input
TEST_F(MedialAxisUtilitiesTest, EmptyInput) {
    std::vector<std::vector<Point2D>> chains;
    std::vector<std::vector<double>> clearances;

    auto result = sampleMedialAxisPaths(chains, clearances);

    EXPECT_TRUE(result.empty());
}

// Test mismatched input sizes
TEST_F(MedialAxisUtilitiesTest, MismatchedInputSizes) {
    std::vector<std::vector<Point2D>> chains = {{Point2D(0, 0), Point2D(10, 0)}};
    std::vector<std::vector<double>> clearances;  // Empty - mismatch

    auto result = sampleMedialAxisPaths(chains, clearances);

    EXPECT_TRUE(result.empty());
}

// Test single point chain
TEST_F(MedialAxisUtilitiesTest, SinglePointChain) {
    std::vector<std::vector<Point2D>> chains = {{Point2D(5.0, 3.0)}};
    std::vector<std::vector<double>> clearances = {{2.5}};

    auto result = sampleMedialAxisPaths(chains, clearances);

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].points.size(), 1);
    EXPECT_TRUE(pointsEqual(result[0].points[0].position, Point2D(5.0, 3.0)));
    EXPECT_DOUBLE_EQ(result[0].points[0].clearanceRadius, 2.5);
    EXPECT_DOUBLE_EQ(result[0].totalLength, 0.0);
}

// Test simple straight line - should include endpoints
TEST_F(MedialAxisUtilitiesTest, StraightLineEndpoints) {
    std::vector<std::vector<Point2D>> chains = {{Point2D(0, 0), Point2D(10, 0)}};
    std::vector<std::vector<double>> clearances = {{0.0, 0.0}};  // Zero clearance at endpoints

    auto result = sampleMedialAxisPaths(chains, clearances, 1.0);

    ASSERT_EQ(result.size(), 1);
    ASSERT_GE(result[0].points.size(), 2);  // At least endpoints

    // Check endpoints are included
    EXPECT_TRUE(pointsEqual(result[0].points.front().position, Point2D(0, 0)));
    EXPECT_TRUE(pointsEqual(result[0].points.back().position, Point2D(10, 0)));
    EXPECT_DOUBLE_EQ(result[0].points.front().clearanceRadius, 0.0);
    EXPECT_DOUBLE_EQ(result[0].points.back().clearanceRadius, 0.0);
}

// Test interpolation on long segments
TEST_F(MedialAxisUtilitiesTest, LongSegmentInterpolation) {
    // 5mm segment should get interpolated points
    std::vector<std::vector<Point2D>> chains = {{Point2D(0, 0), Point2D(5, 0)}};
    std::vector<std::vector<double>> clearances = {{1.0, 2.0}};

    auto result = sampleMedialAxisPaths(chains, clearances, 1.0);

    ASSERT_EQ(result.size(), 1);
    ASSERT_GT(result[0].points.size(), 2);  // Should have interpolated points

    // Check that clearances are interpolated
    for (size_t i = 1; i < result[0].points.size() - 1; ++i) {
        EXPECT_GT(result[0].points[i].clearanceRadius, 1.0);
        EXPECT_LT(result[0].points[i].clearanceRadius, 2.0);
    }
}

// Test multiple chains (like triangle with branches)
TEST_F(MedialAxisUtilitiesTest, MultipleChains) {
    std::vector<std::vector<Point2D>> chains = {
        {Point2D(0, 0), Point2D(5, 5), Point2D(10, 0)},  // First branch
        {Point2D(5, 5), Point2D(5, 10)}           // Second branch
    };
    std::vector<std::vector<double>> clearances = {{0.0, 2.0, 0.0}, {2.0, 0.0}};

    auto result = sampleMedialAxisPaths(chains, clearances, 1.0);

    ASSERT_EQ(result.size(), 2);

    // Check first path
    EXPECT_GE(result[0].points.size(), 3);
    EXPECT_TRUE(pointsEqual(result[0].points.front().position, Point2D(0, 0)));
    EXPECT_TRUE(pointsEqual(result[0].points.back().position, Point2D(10, 0)));

    // Check second path
    EXPECT_GE(result[1].points.size(), 2);
    EXPECT_TRUE(pointsEqual(result[1].points.front().position, Point2D(5, 5)));
    EXPECT_TRUE(pointsEqual(result[1].points.back().position, Point2D(5, 10)));
}

// Test regular spacing with different target spacing
TEST_F(MedialAxisUtilitiesTest, DifferentTargetSpacing) {
    std::vector<std::vector<Point2D>> chains = {{Point2D(0, 0), Point2D(10, 0)}};
    std::vector<std::vector<double>> clearances = {{1.0, 1.0}};

    // Test with 2mm spacing
    auto result1 = sampleMedialAxisPaths(chains, clearances, 2.0);

    // Test with 0.5mm spacing
    auto result2 = sampleMedialAxisPaths(chains, clearances, 0.5);

    // More points with smaller spacing
    EXPECT_LT(result1[0].points.size(), result2[0].points.size());

    // Both should have correct total length
    EXPECT_DOUBLE_EQ(result1[0].totalLength, 10.0);
    EXPECT_DOUBLE_EQ(result2[0].totalLength, 10.0);
}

// Test clearance preservation
TEST_F(MedialAxisUtilitiesTest, ClearancePreservation) {
    // Create a path with varying clearances
    std::vector<std::vector<Point2D>> chains = {{Point2D(0, 0), Point2D(5, 0), Point2D(10, 0)}};
    std::vector<std::vector<double>> clearances = {{0.0, 5.0, 0.0}};

    auto result = sampleMedialAxisPaths(chains, clearances, 1.0);

    ASSERT_EQ(result.size(), 1);

    // Check that clearances vary smoothly
    double maxClearance = 0.0;
    for (const auto& point : result[0].points) {
        maxClearance = std::max(maxClearance, point.clearanceRadius);
        EXPECT_GE(point.clearanceRadius, 0.0);
        EXPECT_LE(point.clearanceRadius, 5.0);
    }

    // Should have achieved maximum clearance somewhere
    EXPECT_NEAR(maxClearance, 5.0, 0.1);
}

// Test very short path
TEST_F(MedialAxisUtilitiesTest, VeryShortPath) {
    // 0.5mm path - shorter than interpolation threshold
    std::vector<std::vector<Point2D>> chains = {{Point2D(0, 0), Point2D(0.5, 0)}};
    std::vector<std::vector<double>> clearances = {{1.0, 1.5}};

    auto result = sampleMedialAxisPaths(chains, clearances, 1.0);

    ASSERT_EQ(result.size(), 1);
    // Should only have endpoints (no interpolation for short segments)
    EXPECT_EQ(result[0].points.size(), 2);
    EXPECT_DOUBLE_EQ(result[0].totalLength, 0.5);
}
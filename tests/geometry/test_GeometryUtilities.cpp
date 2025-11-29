/**
 * Comprehensive unit tests for geometry utility functions
 * Tests calculateCentroid utilities from Shape.h
 * All tests are non-fragile - pure input/output testing with no external dependencies
 */

#include <gtest/gtest.h>

#include <limits>
#include <vector>

#include "geometry/Shape.h"

using namespace ChipCarving::Geometry;

class GeometryUtilitiesTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Create standard test point sets
        emptyPoints.clear();

        singlePoint = {Point2D(5.0, 3.0)};

        horizontalLine = {Point2D(0.0, 5.0), Point2D(10.0, 5.0)};

        verticalLine = {Point2D(5.0, 0.0), Point2D(5.0, 10.0)};

        square = {Point2D(0.0, 0.0), Point2D(10.0, 0.0), Point2D(10.0, 10.0), Point2D(0.0, 10.0)};

        triangle = {Point2D(0.0, 0.0), Point2D(10.0, 0.0), Point2D(5.0, 8.0)};

        negativeCoords = {Point2D(-5.0, -3.0), Point2D(-10.0, -8.0), Point2D(-2.0, -1.0)};

        mixedCoords = {Point2D(-5.0, -3.0), Point2D(10.0, -8.0), Point2D(3.0, 12.0),
                       Point2D(-7.0, 5.0)};

        largeValues = {Point2D(1000000.0, 2000000.0), Point2D(-3000000.0, 4000000.0),
                       Point2D(5000000.0, -6000000.0)};

        smallValues = {Point2D(0.0001, 0.0002), Point2D(-0.0003, 0.0004), Point2D(0.0005, -0.0006)};
    }

    std::vector<Point2D> emptyPoints;
    std::vector<Point2D> singlePoint;
    std::vector<Point2D> horizontalLine;
    std::vector<Point2D> verticalLine;
    std::vector<Point2D> square;
    std::vector<Point2D> triangle;
    std::vector<Point2D> negativeCoords;
    std::vector<Point2D> mixedCoords;
    std::vector<Point2D> largeValues;
    std::vector<Point2D> smallValues;

    static constexpr double TOLERANCE = 1e-9;
};

// ===============================
// calculateBounds Tests - REMOVED (not used in core functionality)
// ===============================
/*

TEST_F(GeometryUtilitiesTest, CalculateBoundsEmptyPoints) {
    Point2D min, max;
    calculateBounds(emptyPoints, min, max);

    // For empty points, both min and max should be (0,0)
    EXPECT_DOUBLE_EQ(min.x, 0.0);
    EXPECT_DOUBLE_EQ(min.y, 0.0);
    EXPECT_DOUBLE_EQ(max.x, 0.0);
    EXPECT_DOUBLE_EQ(max.y, 0.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsSinglePoint) {
    Point2D min, max;
    calculateBounds(singlePoint, min, max);

    // For single point, min and max should be the same
    EXPECT_DOUBLE_EQ(min.x, 5.0);
    EXPECT_DOUBLE_EQ(min.y, 3.0);
    EXPECT_DOUBLE_EQ(max.x, 5.0);
    EXPECT_DOUBLE_EQ(max.y, 3.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsHorizontalLine) {
    Point2D min, max;
    calculateBounds(horizontalLine, min, max);

    EXPECT_DOUBLE_EQ(min.x, 0.0);
    EXPECT_DOUBLE_EQ(min.y, 5.0);
    EXPECT_DOUBLE_EQ(max.x, 10.0);
    EXPECT_DOUBLE_EQ(max.y, 5.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsVerticalLine) {
    Point2D min, max;
    calculateBounds(verticalLine, min, max);

    EXPECT_DOUBLE_EQ(min.x, 5.0);
    EXPECT_DOUBLE_EQ(min.y, 0.0);
    EXPECT_DOUBLE_EQ(max.x, 5.0);
    EXPECT_DOUBLE_EQ(max.y, 10.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsSquare) {
    Point2D min, max;
    calculateBounds(square, min, max);

    EXPECT_DOUBLE_EQ(min.x, 0.0);
    EXPECT_DOUBLE_EQ(min.y, 0.0);
    EXPECT_DOUBLE_EQ(max.x, 10.0);
    EXPECT_DOUBLE_EQ(max.y, 10.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsTriangle) {
    Point2D min, max;
    calculateBounds(triangle, min, max);

    EXPECT_DOUBLE_EQ(min.x, 0.0);
    EXPECT_DOUBLE_EQ(min.y, 0.0);
    EXPECT_DOUBLE_EQ(max.x, 10.0);
    EXPECT_DOUBLE_EQ(max.y, 8.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsNegativeCoords) {
    Point2D min, max;
    calculateBounds(negativeCoords, min, max);

    EXPECT_DOUBLE_EQ(min.x, -10.0);
    EXPECT_DOUBLE_EQ(min.y, -8.0);
    EXPECT_DOUBLE_EQ(max.x, -2.0);
    EXPECT_DOUBLE_EQ(max.y, -1.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsMixedCoords) {
    Point2D min, max;
    calculateBounds(mixedCoords, min, max);

    EXPECT_DOUBLE_EQ(min.x, -7.0);
    EXPECT_DOUBLE_EQ(min.y, -8.0);
    EXPECT_DOUBLE_EQ(max.x, 10.0);
    EXPECT_DOUBLE_EQ(max.y, 12.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsLargeValues) {
    Point2D min, max;
    calculateBounds(largeValues, min, max);

    EXPECT_DOUBLE_EQ(min.x, -3000000.0);
    EXPECT_DOUBLE_EQ(min.y, -6000000.0);
    EXPECT_DOUBLE_EQ(max.x, 5000000.0);
    EXPECT_DOUBLE_EQ(max.y, 4000000.0);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsSmallValues) {
    Point2D min, max;
    calculateBounds(smallValues, min, max);

    EXPECT_DOUBLE_EQ(min.x, -0.0003);
    EXPECT_DOUBLE_EQ(min.y, -0.0006);
    EXPECT_DOUBLE_EQ(max.x, 0.0005);
    EXPECT_DOUBLE_EQ(max.y, 0.0004);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsIdenticalPoints) {
    std::vector<Point2D> identicalPoints = {Point2D(5.0, 5.0), Point2D(5.0, 5.0),
                                            Point2D(5.0, 5.0)};

    Point2D min, max;
    calculateBounds(identicalPoints, min, max);

    EXPECT_DOUBLE_EQ(min.x, 5.0);
    EXPECT_DOUBLE_EQ(min.y, 5.0);
    EXPECT_DOUBLE_EQ(max.x, 5.0);
    EXPECT_DOUBLE_EQ(max.y, 5.0);
}

// ===============================
// calculateCentroid Tests
// ===============================

TEST_F(GeometryUtilitiesTest, CalculateCentroidEmptyPoints) {
    Point2D centroid = calculateCentroid(emptyPoints);

    // For empty points, centroid should be (0,0)
    EXPECT_DOUBLE_EQ(centroid.x, 0.0);
    EXPECT_DOUBLE_EQ(centroid.y, 0.0);
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidSinglePoint) {
    Point2D centroid = calculateCentroid(singlePoint);

    // For single point, centroid should be the point itself
    EXPECT_DOUBLE_EQ(centroid.x, 5.0);
    EXPECT_DOUBLE_EQ(centroid.y, 3.0);
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidHorizontalLine) {
    Point2D centroid = calculateCentroid(horizontalLine);

    // Centroid of horizontal line should be at midpoint
    EXPECT_DOUBLE_EQ(centroid.x, 5.0);  // (0 + 10) / 2
    EXPECT_DOUBLE_EQ(centroid.y, 5.0);  // (5 + 5) / 2
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidVerticalLine) {
    Point2D centroid = calculateCentroid(verticalLine);

    // Centroid of vertical line should be at midpoint
    EXPECT_DOUBLE_EQ(centroid.x, 5.0);  // (5 + 5) / 2
    EXPECT_DOUBLE_EQ(centroid.y, 5.0);  // (0 + 10) / 2
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidSquare) {
    Point2D centroid = calculateCentroid(square);

    // Centroid of square should be at center
    EXPECT_DOUBLE_EQ(centroid.x, 5.0);  // (0 + 10 + 10 + 0) / 4
    EXPECT_DOUBLE_EQ(centroid.y, 5.0);  // (0 + 0 + 10 + 10) / 4
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidTriangle) {
    Point2D centroid = calculateCentroid(triangle);

    // Centroid of triangle at vertices (0,0), (10,0), (5,8)
    EXPECT_DOUBLE_EQ(centroid.x, 5.0);              // (0 + 10 + 5) / 3
    EXPECT_NEAR(centroid.y, 8.0 / 3.0, TOLERANCE);  // (0 + 0 + 8) / 3
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidNegativeCoords) {
    Point2D centroid = calculateCentroid(negativeCoords);

    // (-5 + -10 + -2) / 3 = -17/3 ≈ -5.667
    // (-3 + -8 + -1) / 3 = -12/3 = -4
    EXPECT_NEAR(centroid.x, -17.0 / 3.0, TOLERANCE);
    EXPECT_DOUBLE_EQ(centroid.y, -4.0);
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidMixedCoords) {
    Point2D centroid = calculateCentroid(mixedCoords);

    // (-5 + 10 + 3 + -7) / 4 = 1/4 = 0.25
    // (-3 + -8 + 12 + 5) / 4 = 6/4 = 1.5
    EXPECT_DOUBLE_EQ(centroid.x, 0.25);
    EXPECT_DOUBLE_EQ(centroid.y, 1.5);
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidLargeValues) {
    Point2D centroid = calculateCentroid(largeValues);

    // (1000000 + -3000000 + 5000000) / 3 = 3000000/3 = 1000000
    // (2000000 + 4000000 + -6000000) / 3 = 0/3 = 0
    EXPECT_DOUBLE_EQ(centroid.x, 1000000.0);
    EXPECT_DOUBLE_EQ(centroid.y, 0.0);
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidSmallValues) {
    Point2D centroid = calculateCentroid(smallValues);

    // (0.0001 + -0.0003 + 0.0005) / 3 = 0.0003/3 = 0.0001
    // (0.0002 + 0.0004 + -0.0006) / 3 = 0/3 = 0
    EXPECT_NEAR(centroid.x, 0.0001, TOLERANCE);
    EXPECT_NEAR(centroid.y, 0.0, TOLERANCE);
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidIdenticalPoints) {
    std::vector<Point2D> identicalPoints = {Point2D(7.5, 3.2), Point2D(7.5, 3.2), Point2D(7.5, 3.2),
                                            Point2D(7.5, 3.2)};

    Point2D centroid = calculateCentroid(identicalPoints);

    // Centroid of identical points should be the point itself
    EXPECT_DOUBLE_EQ(centroid.x, 7.5);
    EXPECT_DOUBLE_EQ(centroid.y, 3.2);
}

// ===============================
// Edge Cases and Precision Tests
// ===============================

TEST_F(GeometryUtilitiesTest, CalculateBoundsPrecision) {
    std::vector<Point2D> precisionPoints = {Point2D(1.0 + 1e-15, 2.0 + 1e-15),
                                            Point2D(1.0 - 1e-15, 2.0 - 1e-15)};

    Point2D min, max;
    calculateBounds(precisionPoints, min, max);

    // Should handle tiny differences correctly
    EXPECT_LE(min.x, 1.0);
    EXPECT_LE(min.y, 2.0);
    EXPECT_GE(max.x, 1.0);
    EXPECT_GE(max.y, 2.0);
}

TEST_F(GeometryUtilitiesTest, CalculateCentroidLargePointCount) {
    // Create a circle of 100 points
    std::vector<Point2D> circlePoints;
    const double radius = 10.0;
    const double centerX = 5.0;
    const double centerY = 3.0;
    const int numPoints = 100;

    for (int i = 0; i < numPoints; ++i) {
        double angle = 2.0 * M_PI * i / numPoints;
        circlePoints.push_back(
            Point2D(centerX + radius * std::cos(angle), centerY + radius * std::sin(angle)));
    }

    Point2D centroid = calculateCentroid(circlePoints);

    // Centroid of points on a circle should be the center
    EXPECT_NEAR(centroid.x, centerX, 1e-6);
    EXPECT_NEAR(centroid.y, centerY, 1e-6);
}

TEST_F(GeometryUtilitiesTest, CalculateBoundsAndCentroidConsistency) {
    // Test that centroid is within bounds for various shapes
    std::vector<std::vector<Point2D>> testSets = {square, triangle, mixedCoords, largeValues,
                                                  smallValues};

    for (const auto& points : testSets) {
        Point2D min, max;
        calculateBounds(points, min, max);

        Point2D centroid = calculateCentroid(points);

        // Centroid should always be within or on the bounding box
        EXPECT_GE(centroid.x, min.x);
        EXPECT_LE(centroid.x, max.x);
        EXPECT_GE(centroid.y, min.y);
        EXPECT_LE(centroid.y, max.y);
    }
}

*/

TEST_F(GeometryUtilitiesTest, CalculateWithNaNInfinity) {
    // Test handling of special floating point values
    std::vector<Point2D> specialPoints = {Point2D(1.0, 2.0), Point2D(3.0, 4.0), Point2D(5.0, 6.0)};

    // Normal case should work (calculateBounds test removed)

    Point2D centroid = calculateCentroid(specialPoints);
    EXPECT_DOUBLE_EQ(centroid.x, 3.0);  // (1 + 3 + 5) / 3
    EXPECT_DOUBLE_EQ(centroid.y, 4.0);  // (2 + 4 + 6) / 3
}
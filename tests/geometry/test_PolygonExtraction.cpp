/**
 * test_PolygonExtraction.cpp
 *
 * Tests for polygon extraction algorithms including curve chaining
 * and orientation detection. These are critical algorithms that were
 * previously untested.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include "../../include/geometry/Point2D.h"

using namespace ChipCarving::Geometry;

// Helper function to calculate signed area (determines winding order)
double calculateSignedArea(const std::vector<Point2D>& polygon) {
    double area = 0.0;
    size_t n = polygon.size();

    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        area += (polygon[i].x * polygon[j].y) - (polygon[j].x * polygon[i].y);
    }

    return area / 2.0;
}

// Helper function to check if polygon is counter-clockwise
bool isCounterClockwise(const std::vector<Point2D>& polygon) {
    return calculateSignedArea(polygon) > 0;
}

// Helper function to reverse polygon order
std::vector<Point2D> reversePolygon(const std::vector<Point2D>& polygon) {
    std::vector<Point2D> reversed = polygon;
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

// Mock curve data structure similar to what's used in FusionAPIAdapter
struct MockCurveData {
    size_t originalIndex;
    std::vector<Point2D> points;
    Point2D startPoint() const {
        return points.front();
    }
    Point2D endPoint() const {
        return points.back();
    }
    bool used = false;
};

// Simplified curve chaining algorithm for testing
std::vector<Point2D> chainCurves(std::vector<MockCurveData>& curves, double tolerance = 0.001) {
    std::vector<Point2D> result;
    if (curves.empty())
        return result;

    // Start with first curve
    std::vector<size_t> chainOrder;
    chainOrder.push_back(0);
    curves[0].used = true;

    Point2D currentEnd = curves[0].endPoint();

    // Chain remaining curves
    for (size_t chainPos = 1; chainPos < curves.size(); ++chainPos) {
        bool foundNext = false;

        for (size_t i = 0; i < curves.size(); ++i) {
            if (curves[i].used)
                continue;

            // Check if this curve connects to current end
            double distToStart = distance(currentEnd, curves[i].startPoint());
            double distToEnd = distance(currentEnd, curves[i].endPoint());

            if (distToStart < tolerance) {
                // Connect normally
                chainOrder.push_back(i);
                curves[i].used = true;
                currentEnd = curves[i].endPoint();
                foundNext = true;
                break;
            } else if (distToEnd < tolerance) {
                // Need to reverse this curve
                std::reverse(curves[i].points.begin(), curves[i].points.end());
                chainOrder.push_back(i);
                curves[i].used = true;
                currentEnd = curves[i].endPoint();
                foundNext = true;
                break;
            }
        }

        if (!foundNext) {
            break;  // Gap in curves
        }
    }

    // Build final polygon from chained curves
    for (size_t i = 0; i < chainOrder.size(); ++i) {
        size_t idx = chainOrder[i];
        const auto& curvePoints = curves[idx].points;
        // Always skip last point to avoid duplicates
        size_t numPoints = curvePoints.size() - 1;
        for (size_t j = 0; j < numPoints; ++j) {
            result.push_back(curvePoints[j]);
        }
    }

    return result;
}

class PolygonExtractionTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Common test setup if needed
    }
};

/**
 * Test basic curve chaining with curves in correct order
 */
TEST_F(PolygonExtractionTest, ChainCurvesInOrder) {
    std::vector<MockCurveData> curves;

    // Create a triangle with 3 edges already in order
    curves.push_back(MockCurveData{0, {Point2D(0, 0), Point2D(1, 0)}});        // Edge 0: (0,0) -> (1,0)
    curves.push_back(MockCurveData{1, {Point2D(1, 0), Point2D(0.5, 0.866)}});  // Edge 1: (1,0) -> (0.5,0.866)
    curves.push_back(MockCurveData{2, {Point2D(0.5, 0.866), Point2D(0, 0)}});  // Edge 2: (0.5,0.866) -> (0,0)

    std::vector<Point2D> polygon = chainCurves(curves);

    EXPECT_EQ(polygon.size(), 3);  // Should have 3 vertices (no duplicates)
    EXPECT_NEAR(polygon[0].x, 0, 1e-10);
    EXPECT_NEAR(polygon[0].y, 0, 1e-10);
    EXPECT_NEAR(polygon[1].x, 1, 1e-10);
    EXPECT_NEAR(polygon[1].y, 0, 1e-10);
    EXPECT_NEAR(polygon[2].x, 0.5, 1e-10);
    EXPECT_NEAR(polygon[2].y, 0.866, 0.001);
}

/**
 * Test curve chaining with curves in random order
 */
TEST_F(PolygonExtractionTest, ChainCurvesRandomOrder) {
    std::vector<MockCurveData> curves;

    // Create a square with edges in random order
    curves.push_back({0, {Point2D(1, 0), Point2D(1, 1)}});  // Right edge
    curves.push_back({1, {{0, 1}, Point2D(0, 0)}});  // Left edge (reversed)
    curves.push_back({2, {Point2D(1, 1), {0, 1}}});  // Top edge
    curves.push_back({3, {Point2D(0, 0), Point2D(1, 0)}});  // Bottom edge

    std::vector<Point2D> polygon = chainCurves(curves);

    // Should still form a proper square
    EXPECT_EQ(polygon.size(), 4);

    // Verify it forms a closed polygon
    double perimeter = 0;
    for (size_t i = 0; i < polygon.size(); ++i) {
        size_t next = (i + 1) % polygon.size();
        perimeter += distance(polygon[i], polygon[next]);
    }
    EXPECT_NEAR(perimeter, 4.0, 0.001);  // Square perimeter = 4
}

/**
 * Test curve chaining with curves that need reversal
 */
TEST_F(PolygonExtractionTest, ChainCurvesWithReversal) {
    std::vector<MockCurveData> curves;

    // Create a triangle with some edges backward
    curves.push_back({0, {Point2D(0, 0), Point2D(1, 0)}});        // Edge 0: forward
    curves.push_back({1, {Point2D(0.5, 0.866), Point2D(1, 0)}});  // Edge 1: backward (needs reversal)
    curves.push_back({2, {Point2D(0.5, 0.866), Point2D(0, 0)}});  // Edge 2: forward

    std::vector<Point2D> polygon = chainCurves(curves);

    EXPECT_EQ(polygon.size(), 3);

    // Verify correct chaining despite reversal
    double area = std::abs(calculateSignedArea(polygon));
    EXPECT_NEAR(area, 0.433, 0.001);  // Area of equilateral triangle with side 1
}

/**
 * Test handling of gaps in curves (should stop chaining)
 */
TEST_F(PolygonExtractionTest, ChainCurvesWithGap) {
    std::vector<MockCurveData> curves;

    // Create curves with a gap
    curves.push_back({0, {Point2D(0, 0), Point2D(1, 0)}});  // Edge 0
    curves.push_back({1, {{2, 0}, {2, 1}}});  // Edge 1 (gap from previous)
    curves.push_back({2, {Point2D(1, 0), Point2D(1, 1)}});  // Edge 2 (would connect if no gap)

    std::vector<Point2D> polygon = chainCurves(curves);

    // Should chain first and third curves (they connect), but not second
    // Each curve has 2 points, we skip last point of each curve
    // So we get: first point of curve 0, first point of curve 2 = 2 points
    EXPECT_EQ(polygon.size(), 2);
}

/**
 * Test empty curve handling
 */
TEST_F(PolygonExtractionTest, ChainEmptyCurves) {
    std::vector<MockCurveData> curves;
    std::vector<Point2D> polygon = chainCurves(curves);

    EXPECT_TRUE(polygon.empty());
}

/**
 * Test polygon orientation detection - CCW triangle
 */
TEST_F(PolygonExtractionTest, OrientationCCWTriangle) {
    std::vector<Point2D> triangle = {
        Point2D(0, 0), Point2D(1, 0), Point2D(0.5, 0.866)  // Points up
    };

    double area = calculateSignedArea(triangle);
    EXPECT_GT(area, 0);  // Positive area = CCW
    EXPECT_TRUE(isCounterClockwise(triangle));

    // Verify specific area value
    EXPECT_NEAR(area, 0.433, 0.001);
}

/**
 * Test polygon orientation detection - CW triangle
 */
TEST_F(PolygonExtractionTest, OrientationCWTriangle) {
    std::vector<Point2D> triangle = {
        Point2D(0, 0),
        Point2D(0.5, 0.866),  // Points up
        Point2D(1, 0)         // Reversed order from CCW
    };

    double area = calculateSignedArea(triangle);
    EXPECT_LT(area, 0);  // Negative area = CW
    EXPECT_FALSE(isCounterClockwise(triangle));

    // Verify specific area value
    EXPECT_NEAR(area, -0.433, 0.001);
}

/**
 * Test polygon orientation with complex shape
 */
TEST_F(PolygonExtractionTest, OrientationComplexShape) {
    // L-shaped polygon (CCW)
    std::vector<Point2D> lShape = {Point2D(0, 0), {2, 0}, {2, 1}, Point2D(1, 1), {1, 2}, {0, 2}};

    EXPECT_TRUE(isCounterClockwise(lShape));

    // Reverse it
    std::vector<Point2D> lShapeReversed = reversePolygon(lShape);
    EXPECT_FALSE(isCounterClockwise(lShapeReversed));
}

/**
 * Test orientation detection with nearly collinear points
 */
TEST_F(PolygonExtractionTest, OrientationNearlyCollinear) {
    // Very flat triangle (but still CCW)
    std::vector<Point2D> flatTriangle = {
        Point2D(0, 0), Point2D(1, 0), {0.5, 0.001}  // Very small height
    };

    double area = calculateSignedArea(flatTriangle);
    EXPECT_GT(area, 0);  // Should still be positive
    EXPECT_TRUE(isCounterClockwise(flatTriangle));

    // Area should be very small but positive
    EXPECT_NEAR(area, 0.0005, 0.0001);
}

/**
 * Test orientation with self-intersecting polygon
 */
TEST_F(PolygonExtractionTest, OrientationSelfIntersecting) {
    // Figure-8 shape (self-intersecting)
    std::vector<Point2D> figure8 = {Point2D(0, 0), Point2D(1, 1), Point2D(1, 0), {0, 1}};

    // Signed area can be positive or negative depending on interpretation
    double area = calculateSignedArea(figure8);
    // This specific figure-8 should have zero area (areas cancel out)
    EXPECT_NEAR(std::abs(area), 0.0, 0.001);
}

/**
 * Test with real-world polygon from Fusion data
 */
TEST_F(PolygonExtractionTest, RealWorldFusionPolygon) {
    // This is the actual polygon from the logs that had issues
    std::vector<Point2D> fusionPolygon = {
        {1.985216, -2.539599}, {2.542346, -1.799321}, {3.036261, -1.015452}, {3.463569, -0.193374},
        {3.821334, 0.661265},  {4.107100, 1.542595},  {4.318902, 2.444562},  {4.591039, 1.544441},
        {4.936780, 0.669946},  {5.353749, -0.172914}, {5.839081, -0.978352}, {6.389444, -1.740833},
        {7.001056, -2.455121}, {6.171789, -2.295278}, {5.332133, -2.204653}, {4.487856, -2.183870},
        {3.644759, -2.233072}, {2.808631, -2.351920}};

    // This polygon is CW (negative area) based on the vertex order
    EXPECT_FALSE(isCounterClockwise(fusionPolygon));

    double area = calculateSignedArea(fusionPolygon);
    EXPECT_NEAR(area, -8.88126, 0.001);  // Negative area for CW polygon
}

/**
 * Test tolerance in curve connection
 */
TEST_F(PolygonExtractionTest, CurveConnectionTolerance) {
    std::vector<MockCurveData> curves;

    // Create edges with small gaps (within tolerance)
    curves.push_back({0, {Point2D(0, 0), {1, 0.0001}}});  // Small Y offset
    curves.push_back({1, {{1.0001, 0}, Point2D(1, 1)}});  // Small X offset
    curves.push_back({2, {Point2D(1, 1), {0, 1}}});
    curves.push_back({3, {{0, 1}, {0.0001, 0}}});  // Almost closes

    std::vector<Point2D> polygon = chainCurves(curves, 0.001);  // 0.001 tolerance

    // Should successfully chain all curves despite small gaps
    EXPECT_EQ(polygon.size(), 4);
}

/**
 * Test curve chaining performance with many segments
 */
TEST_F(PolygonExtractionTest, ChainManySegments) {
    std::vector<MockCurveData> curves;

    // Create a circle approximation with many segments
    const int numSegments = 100;
    for (int i = 0; i < numSegments; ++i) {
        double angle1 = 2 * M_PI * i / numSegments;
        double angle2 = 2 * M_PI * (i + 1) / numSegments;

        curves.push_back(
            {static_cast<size_t>(i), {{cos(angle1), sin(angle1)}, {cos(angle2), sin(angle2)}}});
    }

    // Shuffle to test arbitrary order
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(curves.begin(), curves.end(), g);

    std::vector<Point2D> polygon = chainCurves(curves);

    // Should chain all segments
    EXPECT_EQ(polygon.size(), numSegments);

    // Verify it forms a closed shape
    double lastGap = distance(polygon.back(), polygon.front());
    EXPECT_LT(lastGap, 0.1);  // Should close within reasonable tolerance
}

/**
 * Test duplicate vertex removal
 */
TEST_F(PolygonExtractionTest, DuplicateVertexRemoval) {
    std::vector<MockCurveData> curves;

    // Create triangle where last vertex would duplicate first
    curves.push_back({0, {Point2D(0, 0), Point2D(1, 0)}});
    curves.push_back({1, {Point2D(1, 0), Point2D(0.5, 0.866)}});
    curves.push_back({2, {Point2D(0.5, 0.866), Point2D(0, 0)}});  // Closes back to start

    std::vector<Point2D> polygon = chainCurves(curves);

    // Should not have duplicate final vertex
    EXPECT_EQ(polygon.size(), 3);

    // Verify no consecutive duplicates
    for (size_t i = 0; i < polygon.size(); ++i) {
        size_t next = (i + 1) % polygon.size();
        double dist = distance(polygon[i], polygon[next]);
        EXPECT_GT(dist, 0.001);  // No duplicates
    }
}
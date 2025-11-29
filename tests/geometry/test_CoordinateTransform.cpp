/**
 * test_CoordinateTransform.cpp
 *
 * Tests for coordinate transformation accuracy and precision,
 * particularly the unit circle transformation used in MedialAxisProcessor.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "geometry/Point2D.h"

using namespace ChipCarving::Geometry;

// Transformation parameters structure
struct TransformParams {
    Point2D offset;
    double scale;
    Point2D originalMin;
    Point2D originalMax;
};

// Helper functions to mimic MedialAxisProcessor transformations
void calculateBounds(const std::vector<Point2D>& points, Point2D& min, Point2D& max) {
    if (points.empty())
        return;

    min = max = points[0];
    for (const auto& p : points) {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
    }
}

TransformParams calculateTransformToUnitCircle(const std::vector<Point2D>& polygon) {
    TransformParams params;

    calculateBounds(polygon, params.originalMin, params.originalMax);

    Point2D center((params.originalMin.x + params.originalMax.x) * 0.5,
                   (params.originalMin.y + params.originalMax.y) * 0.5);
    double width = params.originalMax.x - params.originalMin.x;
    double height = params.originalMax.y - params.originalMin.y;
    double maxDimension = std::max(width, height);

    params.offset = Point2D(-center.x, -center.y);

    // Handle degenerate case where all points are the same
    if (maxDimension < 1e-10) {
        params.scale = 1.0;  // Default scale for zero-size polygon
    } else {
        params.scale = 0.95 / maxDimension;  // 0.95 to leave margin
    }

    return params;
}

Point2D transformToUnitCircle(const Point2D& point, const TransformParams& params) {
    Point2D translated = point + params.offset;
    return Point2D(translated.x * params.scale, translated.y * params.scale);
}

Point2D transformFromUnitCircle(const Point2D& unitPoint, const TransformParams& params) {
    Point2D scaled(unitPoint.x / params.scale, unitPoint.y / params.scale);
    return scaled - params.offset;
}

class CoordinateTransformTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Set up random number generator for some tests
        rng.seed(12345);  // Fixed seed for reproducibility
    }

    std::mt19937 rng;
    std::uniform_real_distribution<double> coordDist{-1000.0, 1000.0};
};

/**
 * Test basic transform calculation
 */
TEST_F(CoordinateTransformTest, BasicTransformCalculation) {
    std::vector<Point2D> square = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};

    TransformParams params = calculateTransformToUnitCircle(square);

    // Center should be at (5, 5)
    EXPECT_NEAR(params.offset.x, -5.0, 1e-10);
    EXPECT_NEAR(params.offset.y, -5.0, 1e-10);

    // Scale should fit 10x10 square into 0.95 radius
    EXPECT_NEAR(params.scale, 0.095, 1e-10);  // 0.95 / 10

    // Original bounds
    EXPECT_EQ(params.originalMin.x, 0);
    EXPECT_EQ(params.originalMin.y, 0);
    EXPECT_EQ(params.originalMax.x, 10);
    EXPECT_EQ(params.originalMax.y, 10);
}

/**
 * Test transform to unit circle
 */
TEST_F(CoordinateTransformTest, TransformToUnitCircle) {
    std::vector<Point2D> rect = {{100, 200}, {300, 200}, {300, 400}, {100, 400}};

    TransformParams params = calculateTransformToUnitCircle(rect);

    // Transform all points
    std::vector<Point2D> transformed;
    for (const auto& p : rect) {
        transformed.push_back(transformToUnitCircle(p, params));
    }

    // All transformed points should be within unit circle
    for (const auto& p : transformed) {
        double radius = std::sqrt(p.x * p.x + p.y * p.y);
        EXPECT_LE(radius, 0.95);  // Should be within 0.95 radius
    }

    // The rectangle has width 200, height 200, so it's a square
    // When centered and scaled, the corners will be at radius sqrt(2) * 0.475 ≈ 0.672
    double maxRadius = 0;
    for (const auto& p : transformed) {
        double radius = std::sqrt(p.x * p.x + p.y * p.y);
        maxRadius = std::max(maxRadius, radius);
    }
    // Corners should be at sqrt(0.475^2 + 0.475^2) ≈ 0.672
    EXPECT_NEAR(maxRadius, 0.672, 0.01);
}

/**
 * Test round-trip transformation precision
 */
TEST_F(CoordinateTransformTest, RoundTripPrecision) {
    // Test with various polygons
    std::vector<std::vector<Point2D>> testPolygons = {
        // Small integer coordinates
        {{0, 0}, {1, 0}, {1, 1}, {0, 1}},

        // Large coordinates
        {{1000, 2000}, {3000, 2000}, {2000, 4000}},

        // Fractional coordinates
        {{0.123456789, 0.987654321}, {1.111111111, 2.222222222}, {3.333333333, 1.444444444}},

        // Mixed positive/negative
        {{-100, -100}, {100, -100}, {100, 100}, {-100, 100}},

        // Very small coordinates
        {{0.001, 0.001}, {0.002, 0.001}, {0.0015, 0.002}}};

    for (const auto& polygon : testPolygons) {
        TransformParams params = calculateTransformToUnitCircle(polygon);

        // Transform and inverse transform each point
        for (const auto& original : polygon) {
            Point2D unit = transformToUnitCircle(original, params);
            Point2D recovered = transformFromUnitCircle(unit, params);

            // Should match within machine precision
            EXPECT_NEAR(recovered.x, original.x, std::abs(original.x) * 1e-14);
            EXPECT_NEAR(recovered.y, original.y, std::abs(original.y) * 1e-14);
        }
    }
}

/**
 * Test with extreme coordinate values
 */
TEST_F(CoordinateTransformTest, ExtremeCoordinates) {
    // Test with very large coordinates
    double largeVal = 1e15;
    std::vector<Point2D> largePolygon = {
        {0, 0}, {largeVal, 0}, {largeVal, largeVal}, {0, largeVal}};

    TransformParams params = calculateTransformToUnitCircle(largePolygon);

    // Scale should be very small
    EXPECT_LT(params.scale, 1e-14);

    // Transform points
    for (const auto& p : largePolygon) {
        Point2D unit = transformToUnitCircle(p, params);

        // Should be within unit circle
        double radius = std::sqrt(unit.x * unit.x + unit.y * unit.y);
        EXPECT_LE(radius, 0.95);

        // Round trip
        Point2D recovered = transformFromUnitCircle(unit, params);
        EXPECT_NEAR(recovered.x, p.x, std::abs(p.x) * 1e-10);
        EXPECT_NEAR(recovered.y, p.y, std::abs(p.y) * 1e-10);
    }
}

/**
 * Test precision loss with many transformations
 */
TEST_F(CoordinateTransformTest, CumulativePrecisionLoss) {
    Point2D original(123.456789012345, 987.654321098765);
    std::vector<Point2D> polygon = {{100, 900}, {200, 900}, {150, 1000}};

    TransformParams params = calculateTransformToUnitCircle(polygon);

    // Perform multiple round-trip transformations
    Point2D current = original;
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        Point2D unit = transformToUnitCircle(current, params);
        current = transformFromUnitCircle(unit, params);
    }

    // Error should not accumulate significantly
    double errorX = std::abs(current.x - original.x);
    double errorY = std::abs(current.y - original.y);

    EXPECT_LT(errorX, 1e-10);
    EXPECT_LT(errorY, 1e-10);
}

/**
 * Test with random coordinates
 */
TEST_F(CoordinateTransformTest, RandomCoordinates) {
    const int numTests = 100;

    for (int test = 0; test < numTests; ++test) {
        // Generate random triangle
        std::vector<Point2D> triangle;
        for (int i = 0; i < 3; ++i) {
            triangle.push_back({coordDist(rng), coordDist(rng)});
        }

        TransformParams params = calculateTransformToUnitCircle(triangle);

        // Test each vertex
        for (const auto& vertex : triangle) {
            Point2D unit = transformToUnitCircle(vertex, params);

            // Should be within unit circle
            double radius = std::sqrt(unit.x * unit.x + unit.y * unit.y);
            EXPECT_LE(radius, 0.96);  // Small margin for numerical errors

            // Round trip should be accurate
            Point2D recovered = transformFromUnitCircle(unit, params);
            double error = distance(vertex, recovered);
            EXPECT_LT(error, 1e-10);
        }
    }
}

/**
 * Test degenerate cases
 */
TEST_F(CoordinateTransformTest, DegenerateCases) {
    // Single point (all vertices same)
    std::vector<Point2D> singlePoint = {{5, 5}, {5, 5}, {5, 5}};

    TransformParams params = calculateTransformToUnitCircle(singlePoint);

    // Scale should be set to safe default (not infinity)
    EXPECT_GT(params.scale, 0);
    EXPECT_FALSE(std::isinf(params.scale));

    // Very thin rectangle
    std::vector<Point2D> thinRect = {{0, 0}, {1000, 0}, {1000, 0.001}, {0, 0.001}};

    params = calculateTransformToUnitCircle(thinRect);

    // Should scale based on larger dimension
    EXPECT_NEAR(params.scale, 0.95 / 1000, 1e-6);
}

/**
 * Test preservation of relative positions
 */
TEST_F(CoordinateTransformTest, RelativePositionPreservation) {
    std::vector<Point2D> polygon = {{10, 20}, {30, 20}, {20, 40}};

    TransformParams params = calculateTransformToUnitCircle(polygon);

    // Transform all points
    std::vector<Point2D> transformed;
    for (const auto& p : polygon) {
        transformed.push_back(transformToUnitCircle(p, params));
    }

    // Check that relative positions are preserved
    // Original: p0 to p1 is horizontal
    Point2D origDiff01 = polygon[1] - polygon[0];
    Point2D transDiff01 = transformed[1] - transformed[0];

    // Directions should be same (accounting for scale)
    double origAngle01 = std::atan2(origDiff01.y, origDiff01.x);
    double transAngle01 = std::atan2(transDiff01.y, transDiff01.x);
    EXPECT_NEAR(origAngle01, transAngle01, 1e-10);

    // Relative distances should be preserved
    double origDist01 = distance(polygon[0], polygon[1]);
    double origDist12 = distance(polygon[1], polygon[2]);
    double transDist01 = distance(transformed[0], transformed[1]);
    double transDist12 = distance(transformed[1], transformed[2]);

    EXPECT_NEAR(origDist01 / origDist12, transDist01 / transDist12, 1e-10);
}

/**
 * Test numerical stability near zero
 */
TEST_F(CoordinateTransformTest, NumericalStabilityNearZero) {
    // Polygon centered near origin with small dimensions
    double epsilon = std::numeric_limits<double>::epsilon();
    std::vector<Point2D> tinyPolygon = {{-epsilon, -epsilon}, {epsilon, -epsilon}, {0, epsilon}};

    TransformParams params = calculateTransformToUnitCircle(tinyPolygon);

    // Should handle without numerical issues
    EXPECT_FALSE(std::isnan(params.scale));
    EXPECT_FALSE(std::isinf(params.scale));
    EXPECT_GT(params.scale, 0);

    // Transform should work
    for (const auto& p : tinyPolygon) {
        Point2D unit = transformToUnitCircle(p, params);
        EXPECT_FALSE(std::isnan(unit.x));
        EXPECT_FALSE(std::isnan(unit.y));

        Point2D recovered = transformFromUnitCircle(unit, params);
        EXPECT_NEAR(recovered.x, p.x, 1e-15);
        EXPECT_NEAR(recovered.y, p.y, 1e-15);
    }
}

/**
 * Test that transform parameters are consistent
 */
TEST_F(CoordinateTransformTest, TransformParameterConsistency) {
    std::vector<Point2D> polygon = {{-10, -20}, {50, -20}, {50, 80}, {-10, 80}};

    // Calculate transform multiple times - should be deterministic
    TransformParams params1 = calculateTransformToUnitCircle(polygon);
    TransformParams params2 = calculateTransformToUnitCircle(polygon);
    TransformParams params3 = calculateTransformToUnitCircle(polygon);

    // All parameters should be identical
    EXPECT_EQ(params1.offset.x, params2.offset.x);
    EXPECT_EQ(params1.offset.y, params2.offset.y);
    EXPECT_EQ(params1.scale, params2.scale);

    EXPECT_EQ(params2.offset.x, params3.offset.x);
    EXPECT_EQ(params2.offset.y, params3.offset.y);
    EXPECT_EQ(params2.scale, params3.scale);
}
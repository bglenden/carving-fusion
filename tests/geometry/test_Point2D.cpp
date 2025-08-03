/**
 * Unit tests for Point2D utilities
 */

#include <gtest/gtest.h>

#include "../../include/geometry/Point2D.h"

using namespace ChipCarving::Geometry;

class Point2DTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Set up test points
        origin = Point2D(0.0, 0.0);
        p1 = Point2D(3.0, 4.0);
        p2 = Point2D(6.0, 8.0);
    }

    Point2D origin;
    Point2D p1;
    Point2D p2;

    static constexpr double TOLERANCE = 1e-9;
};

TEST_F(Point2DTest, Construction) {
    Point2D default_point;
    EXPECT_DOUBLE_EQ(default_point.x, 0.0);
    EXPECT_DOUBLE_EQ(default_point.y, 0.0);

    Point2D custom_point(5.0, -3.0);
    EXPECT_DOUBLE_EQ(custom_point.x, 5.0);
    EXPECT_DOUBLE_EQ(custom_point.y, -3.0);
}

TEST_F(Point2DTest, BasicOperators) {
    Point2D sum = p1 + p2;
    EXPECT_DOUBLE_EQ(sum.x, 9.0);   // 3 + 6
    EXPECT_DOUBLE_EQ(sum.y, 12.0);  // 4 + 8

    Point2D diff = p2 - p1;
    EXPECT_DOUBLE_EQ(diff.x, 3.0);  // 6 - 3
    EXPECT_DOUBLE_EQ(diff.y, 4.0);  // 8 - 4

    Point2D scaled = p1 * 2.0;
    EXPECT_DOUBLE_EQ(scaled.x, 6.0);  // 3 * 2
    EXPECT_DOUBLE_EQ(scaled.y, 8.0);  // 4 * 2
}

TEST_F(Point2DTest, Equality) {
    Point2D p1_copy(3.0, 4.0);
    EXPECT_TRUE(p1.equals(p1_copy));

    Point2D slightly_off(3.0 + 1e-10, 4.0);
    EXPECT_TRUE(p1.equals(slightly_off));  // Within default tolerance

    Point2D far_off(3.1, 4.0);
    EXPECT_FALSE(p1.equals(far_off));
}

TEST_F(Point2DTest, Distance) {
    // Distance from origin to (3,4) should be 5
    EXPECT_DOUBLE_EQ(distance(origin, p1), 5.0);

    // Distance from (3,4) to (6,8) should be 5 (3-4-5 triangle)
    EXPECT_DOUBLE_EQ(distance(p1, p2), 5.0);

    // Distance from point to itself should be 0
    EXPECT_DOUBLE_EQ(distance(p1, p1), 0.0);
}

TEST_F(Point2DTest, Midpoint) {
    Point2D mid = midpoint(p1, p2);
    EXPECT_DOUBLE_EQ(mid.x, 4.5);  // (3 + 6) / 2
    EXPECT_DOUBLE_EQ(mid.y, 6.0);  // (4 + 8) / 2

    Point2D mid_origin = midpoint(origin, p1);
    EXPECT_DOUBLE_EQ(mid_origin.x, 1.5);  // (0 + 3) / 2
    EXPECT_DOUBLE_EQ(mid_origin.y, 2.0);  // (0 + 4) / 2
}

TEST_F(Point2DTest, Perpendicular) {
    // Perpendicular to horizontal line (1,0) should be (0,1)
    Point2D horizontal_start(0.0, 0.0);
    Point2D horizontal_end(1.0, 0.0);
    Point2D perp_horizontal = perpendicular(horizontal_start, horizontal_end);

    EXPECT_NEAR(perp_horizontal.x, 0.0, TOLERANCE);
    EXPECT_NEAR(perp_horizontal.y, 1.0, TOLERANCE);

    // Perpendicular to vertical line (0,1) should be (-1,0)
    Point2D vertical_start(0.0, 0.0);
    Point2D vertical_end(0.0, 1.0);
    Point2D perp_vertical = perpendicular(vertical_start, vertical_end);

    EXPECT_NEAR(perp_vertical.x, -1.0, TOLERANCE);
    EXPECT_NEAR(perp_vertical.y, 0.0, TOLERANCE);

    // Perpendicular vector should be unit length
    Point2D perp = perpendicular(p1, p2);
    double perp_length = std::sqrt(perp.x * perp.x + perp.y * perp.y);
    EXPECT_NEAR(perp_length, 1.0, TOLERANCE);
}

TEST_F(Point2DTest, PerpendicularDegenerate) {
    // Perpendicular of identical points should return zero vector
    Point2D perp_degenerate = perpendicular(p1, p1);
    EXPECT_DOUBLE_EQ(perp_degenerate.x, 0.0);
    EXPECT_DOUBLE_EQ(perp_degenerate.y, 0.0);
}

TEST_F(Point2DTest, RotatePoint) {
    // Rotate (1,0) by 90° around origin should give (0,1)
    Point2D point(1.0, 0.0);
    Point2D rotated = rotatePoint(point, M_PI / 2.0, origin);

    EXPECT_NEAR(rotated.x, 0.0, TOLERANCE);
    EXPECT_NEAR(rotated.y, 1.0, TOLERANCE);

    // Rotate (1,0) by 180° around origin should give (-1,0)
    Point2D rotated_180 = rotatePoint(point, M_PI, origin);

    EXPECT_NEAR(rotated_180.x, -1.0, TOLERANCE);
    EXPECT_NEAR(rotated_180.y, 0.0, TOLERANCE);

    // Rotating around the point itself should return the same point
    Point2D self_rotated = rotatePoint(p1, M_PI / 4.0, p1);
    EXPECT_TRUE(p1.equals(self_rotated, TOLERANCE));
}
/**
 * Unit tests for Leaf shape implementation
 * Tests should match TypeScript behavior from design_program/tests/shapes/Leaf.test.ts
 */

#include <gtest/gtest.h>

#include "../../include/geometry/Leaf.h"

using namespace ChipCarving::Geometry;

class LeafTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Standard test case: two points 10mm apart with default radius
        focus1 = Point2D(0.0, 0.0);
        focus2 = Point2D(10.0, 0.0);
        defaultRadius = 10.0 * 0.65;  // 6.5mm (default calculation)

        // Create leaf with default radius
        leaf = std::make_unique<Leaf>(focus1, focus2);

        // Create leaf with custom radius
        customRadius = 10.0;  // Large enough for valid geometry
        leafCustom = std::make_unique<Leaf>(focus1, focus2, customRadius);
    }

    Point2D focus1;
    Point2D focus2;
    double defaultRadius;
    double customRadius;
    std::unique_ptr<Leaf> leaf;
    std::unique_ptr<Leaf> leafCustom;

    static constexpr double TOLERANCE = 1e-6;
};

TEST_F(LeafTest, Construction) {
    // Test default radius calculation
    EXPECT_NEAR(leaf->getRadius(), defaultRadius, TOLERANCE);
    EXPECT_TRUE(leaf->getFocus1().equals(focus1, TOLERANCE));
    EXPECT_TRUE(leaf->getFocus2().equals(focus2, TOLERANCE));

    // Test custom radius
    EXPECT_NEAR(leafCustom->getRadius(), customRadius, TOLERANCE);
}

TEST_F(LeafTest, DefaultRadiusCalculation) {
    // Default radius should be 0.65 * chord_length (matching TypeScript ShapeFactory)
    Point2D p1(0.0, 0.0);
    Point2D p2(20.0, 0.0);  // 20mm apart
    Leaf leafDefault(p1, p2);

    EXPECT_NEAR(leafDefault.getRadius(), 20.0 * 0.65, TOLERANCE);  // 13.0mm
}

TEST_F(LeafTest, ValidGeometry) {
    // Default leaf should have valid geometry
    EXPECT_TRUE(leaf->isValidGeometry());
    EXPECT_TRUE(leafCustom->isValidGeometry());

    // Invalid case: radius too small for chord length
    Leaf invalidLeaf(focus1, focus2, 4.0);  // radius < chord_length/2
    EXPECT_FALSE(invalidLeaf.isValidGeometry());

    // Edge case: radius exactly half chord length (degenerate circle)
    Leaf edgeLeaf(focus1, focus2, 5.0);  // radius = chord_length/2
    EXPECT_TRUE(edgeLeaf.isValidGeometry());
}

TEST_F(LeafTest, SagittaCalculation) {
    // For default leaf with 10mm chord and 6.5mm radius
    double expectedSagitta = leaf->getSagitta();
    EXPECT_GT(expectedSagitta, 0.0);

    // For custom leaf with 10mm chord and 10mm radius
    double customSagitta = leafCustom->getSagitta();
    EXPECT_LT(customSagitta, expectedSagitta);  // Larger radius = smaller sagitta for same chord

    // For invalid geometry, sagitta should be 0
    Leaf invalidLeaf(focus1, focus2, 4.0);
    EXPECT_NEAR(invalidLeaf.getSagitta(), 0.0, TOLERANCE);
}

TEST_F(LeafTest, ArcCenters) {
    auto centers = leafCustom->getArcCenters();
    Point2D center1 = centers.first;
    Point2D center2 = centers.second;

    // Arc centers should be equidistant from both foci
    EXPECT_NEAR(distance(center1, focus1), customRadius, TOLERANCE);
    EXPECT_NEAR(distance(center1, focus2), customRadius, TOLERANCE);
    EXPECT_NEAR(distance(center2, focus1), customRadius, TOLERANCE);
    EXPECT_NEAR(distance(center2, focus2), customRadius, TOLERANCE);

    // Arc centers should be symmetric about the chord midpoint
    Point2D midpoint = Point2D(5.0, 0.0);  // Midpoint of (0,0) and (10,0)
    Point2D centerMid = Point2D((center1.x + center2.x) / 2.0, (center1.y + center2.y) / 2.0);
    EXPECT_TRUE(midpoint.equals(centerMid, TOLERANCE));
}

TEST_F(LeafTest, ArcParameters) {
    auto arcParams = leafCustom->getArcParameters();
    auto arc1 = arcParams.first;
    auto arc2 = arcParams.second;

    // Both arcs should have the correct radius
    EXPECT_NEAR(arc1.radius, customRadius, TOLERANCE);
    EXPECT_NEAR(arc2.radius, customRadius, TOLERANCE);

    // Arc centers should match getArcCenters()
    auto centers = leafCustom->getArcCenters();
    EXPECT_TRUE(arc1.center.equals(centers.first, TOLERANCE));
    EXPECT_TRUE(arc2.center.equals(centers.second, TOLERANCE));

    // Angles should be valid (between -π and π)
    EXPECT_GE(arc1.startAngle, -M_PI);
    EXPECT_LE(arc1.startAngle, M_PI);
    EXPECT_GE(arc1.endAngle, -M_PI);
    EXPECT_LE(arc1.endAngle, M_PI);
    EXPECT_GE(arc2.startAngle, -M_PI);
    EXPECT_LE(arc2.startAngle, M_PI);
    EXPECT_GE(arc2.endAngle, -M_PI);
    EXPECT_LE(arc2.endAngle, M_PI);
}

TEST_F(LeafTest, Vertices) {
    auto vertices = leaf->getVertices();
    EXPECT_EQ(vertices.size(), 2);
    EXPECT_TRUE(vertices[0].equals(focus1, TOLERANCE));
    EXPECT_TRUE(vertices[1].equals(focus2, TOLERANCE));
}

TEST_F(LeafTest, Centroid) {
    Point2D centroid = leaf->getCentroid();
    Point2D expectedCentroid = midpoint(focus1, focus2);
    EXPECT_TRUE(centroid.equals(expectedCentroid, TOLERANCE));
}

TEST_F(LeafTest, Contains) {
    // Point at the centroid should be inside
    Point2D centroid = leafCustom->getCentroid();
    EXPECT_TRUE(leafCustom->contains(centroid));

    // Points at the foci should be on the boundary (inside)
    EXPECT_TRUE(leafCustom->contains(focus1));
    EXPECT_TRUE(leafCustom->contains(focus2));

    // Point outside both circles should be outside
    Point2D outside(-5.0, 0.0);
    EXPECT_FALSE(leafCustom->contains(outside));

    // Point inside one circle but outside the intersection should be outside
    Point2D insideOne(15.0, 0.0);  // Inside second circle but outside first
    EXPECT_FALSE(leafCustom->contains(insideOne));
}


TEST_F(LeafTest, InvalidGeometryHandling) {
    // Create leaf with invalid geometry (radius too small)
    Leaf invalidLeaf(focus1, focus2, 4.0);

    // Should handle gracefully without crashing

    auto arcParams = invalidLeaf.getArcParameters();
    EXPECT_NEAR(arcParams.first.radius, 0.0, TOLERANCE);
    EXPECT_NEAR(arcParams.second.radius, 0.0, TOLERANCE);
}

TEST_F(LeafTest, VerticalOrientation) {
    // Test with vertical leaf
    Point2D v1(0.0, 0.0);
    Point2D v2(0.0, 10.0);
    Leaf verticalLeaf(v1, v2, 10.0);

    EXPECT_TRUE(verticalLeaf.isValidGeometry());

    auto centers = verticalLeaf.getArcCenters();
    // Arc centers should be on opposite sides of the vertical line
    EXPECT_NE(centers.first.x, centers.second.x);
    EXPECT_NEAR(centers.first.y, centers.second.y, TOLERANCE);  // Same y-coordinate
}

TEST_F(LeafTest, DiagonalOrientation) {
    // Test with diagonal leaf
    Point2D d1(0.0, 0.0);
    Point2D d2(6.0, 8.0);  // 10mm apart (3-4-5 triangle)
    Leaf diagonalLeaf(d1, d2, 8.0);

    EXPECT_TRUE(diagonalLeaf.isValidGeometry());

    auto centers = diagonalLeaf.getArcCenters();
    // Both centers should be valid
    EXPECT_NEAR(distance(centers.first, d1), 8.0, TOLERANCE);
    EXPECT_NEAR(distance(centers.first, d2), 8.0, TOLERANCE);
    EXPECT_NEAR(distance(centers.second, d1), 8.0, TOLERANCE);
    EXPECT_NEAR(distance(centers.second, d2), 8.0, TOLERANCE);
}
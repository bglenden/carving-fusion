/**
 * Comprehensive unit tests for ShapeFactory class
 * Focuses on JSON parsing, parameter validation, and shape creation through public interface
 * All tests are non-fragile - pure input/output testing with no external dependencies
 */

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "../include/geometry/Leaf.h"
#include "../include/geometry/ShapeFactory.h"
#include "../include/geometry/TriArc.h"
#include "../tests/adapters/MockAdapters.h"

using namespace ChipCarving::Geometry;
using namespace ChipCarving::Adapters;

class ShapeFactoryTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Create mock logger for tests that need it
        mockLogger = std::make_shared<MockLogger>();
    }

    void TearDown() override {
        mockLogger.reset();
    }

    std::shared_ptr<MockLogger> mockLogger;
};

// ===============================
// Successful Shape Creation Tests
// ===============================

TEST_F(ShapeFactoryTest, CreateLeafFromValidJson) {
    std::string leafJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}],
        "radius": 8.0
    })";

    auto shape = ShapeFactory::createFromJson(leafJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);

    // Cast to Leaf to verify properties
    auto* leaf = dynamic_cast<Leaf*>(shape.get());
    ASSERT_NE(leaf, nullptr);

    EXPECT_TRUE(leaf->getFocus1().equals(Point2D(0, 0)));
    EXPECT_TRUE(leaf->getFocus2().equals(Point2D(10, 0)));
    EXPECT_DOUBLE_EQ(leaf->getRadius(), 8.0);
}

TEST_F(ShapeFactoryTest, CreateTriArcFromValidJson) {
    std::string triArcJson = R"({
        "type": "TRI_ARC", 
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}, {"x": 5, "y": 8}],
        "curvatures": [-0.5, 0.0, 0.25]
    })";

    auto shape = ShapeFactory::createFromJson(triArcJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);

    // Cast to TriArc to verify properties
    auto* triArc = dynamic_cast<TriArc*>(shape.get());
    ASSERT_NE(triArc, nullptr);

    EXPECT_TRUE(triArc->getVertex(0).equals(Point2D(0, 0)));
    EXPECT_TRUE(triArc->getVertex(1).equals(Point2D(10, 0)));
    EXPECT_TRUE(triArc->getVertex(2).equals(Point2D(5, 8)));
    EXPECT_DOUBLE_EQ(triArc->getBulgeFactor(0), -0.2);  // Clamped from -0.5 to -0.2
    EXPECT_DOUBLE_EQ(triArc->getBulgeFactor(1), 0.0);   // Zero remains zero
    EXPECT_DOUBLE_EQ(triArc->getBulgeFactor(2),
                     -0.2);  // Converted from +0.25 to -0.25, then clamped to -0.2
}

TEST_F(ShapeFactoryTest, CreateLeafWithMinimumRadius) {
    std::string leafJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}],
        "radius": 5.0
    })";  // Exactly chordLength/2 - should be valid

    auto shape = ShapeFactory::createFromJson(leafJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);
    auto* leaf = dynamic_cast<Leaf*>(shape.get());
    ASSERT_NE(leaf, nullptr);
    EXPECT_DOUBLE_EQ(leaf->getRadius(), 5.0);
}

TEST_F(ShapeFactoryTest, CreateLeafWithNegativeCoordinates) {
    std::string leafJson = R"({
        "type": "LEAF",
        "vertices": [{"x": -5.5, "y": -10.0}, {"x": 15.25, "y": 3.75}],
        "radius": 12.5
    })";

    auto shape = ShapeFactory::createFromJson(leafJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);
    auto* leaf = dynamic_cast<Leaf*>(shape.get());
    ASSERT_NE(leaf, nullptr);

    EXPECT_TRUE(leaf->getFocus1().equals(Point2D(-5.5, -10.0)));
    EXPECT_TRUE(leaf->getFocus2().equals(Point2D(15.25, 3.75)));
    EXPECT_DOUBLE_EQ(leaf->getRadius(), 12.5);
}

TEST_F(ShapeFactoryTest, CreateTriArcWithMixedCurvatures) {
    std::string triArcJson = R"({
        "type": "TRI_ARC",
        "vertices": [{"x": 0, "y": 0}, {"x": 6, "y": 0}, {"x": 3, "y": 5}],
        "curvatures": [-0.75, 0.0, 0.5]
    })";  // Mix of negative, zero, and positive curvatures

    auto shape = ShapeFactory::createFromJson(triArcJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);
    auto* triArc = dynamic_cast<TriArc*>(shape.get());
    ASSERT_NE(triArc, nullptr);

    EXPECT_DOUBLE_EQ(triArc->getBulgeFactor(0), -0.2);  // Clamped from -0.75 to -0.2
    EXPECT_DOUBLE_EQ(triArc->getBulgeFactor(1), 0.0);   // Zero remains zero
    EXPECT_DOUBLE_EQ(triArc->getBulgeFactor(2),
                     -0.2);  // Converted from +0.5 to -0.5, then clamped to -0.2
}

// ===============================
// JSON Parsing Error Tests
// ===============================

TEST_F(ShapeFactoryTest, CreateShapeThrowsOnUnknownType) {
    std::string unknownJson = R"({
        "type": "CIRCLE",
        "center": {"x": 0, "y": 0},
        "radius": 5.0
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(unknownJson, mockLogger.get()), std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateShapeThrowsOnMissingType) {
    std::string noTypeJson = R"({
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}],
        "radius": 5.0
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(noTypeJson, mockLogger.get()), std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateShapeThrowsOnMissingVertices) {
    std::string noVerticesJson = R"({
        "type": "LEAF",
        "radius": 5.0
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(noVerticesJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateShapeThrowsOnEmptyVerticesArray) {
    std::string emptyVerticesJson = R"({
        "type": "LEAF",
        "vertices": [],
        "radius": 5.0
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(emptyVerticesJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateLeafThrowsOnMissingRadius) {
    std::string noRadiusJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}]
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(noRadiusJson, mockLogger.get()), std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateTriArcThrowsOnMissingCurvatures) {
    std::string noCurvaturesJson = R"({
        "type": "TRI_ARC",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}, {"x": 5, "y": 8}]
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(noCurvaturesJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateTriArcThrowsOnEmptyCurvaturesArray) {
    std::string emptyCurvaturesJson = R"({
        "type": "TRI_ARC",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}, {"x": 5, "y": 8}],
        "curvatures": []
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(emptyCurvaturesJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateShapeThrowsOnMalformedJson) {
    std::string malformedJson = R"({type: LEAF, vertices: []})";  // Missing quotes

    EXPECT_THROW(ShapeFactory::createFromJson(malformedJson, mockLogger.get()), std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateShapeThrowsOnMalformedVertices) {
    std::string malformedVerticesJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 1.0}],
        "radius": 5.0
    })";  // Missing y coordinate

    EXPECT_THROW(ShapeFactory::createFromJson(malformedVerticesJson, mockLogger.get()),
                 std::runtime_error);
}

// ===============================
// Parameter Validation Error Tests
// ===============================

TEST_F(ShapeFactoryTest, CreateLeafThrowsOnWrongVertexCount) {
    std::string wrongVertexCountJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}],
        "radius": 5.0
    })";  // Only 1 vertex instead of 2

    EXPECT_THROW(ShapeFactory::createFromJson(wrongVertexCountJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateLeafThrowsOnTooManyVertices) {
    std::string tooManyVerticesJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}, {"x": 5, "y": 0}, {"x": 10, "y": 0}],
        "radius": 5.0
    })";  // 3 vertices instead of 2

    EXPECT_THROW(ShapeFactory::createFromJson(tooManyVerticesJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateLeafThrowsOnNegativeRadius) {
    std::string negativeRadiusJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}],
        "radius": -5.0
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(negativeRadiusJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateLeafThrowsOnZeroRadius) {
    std::string zeroRadiusJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}],
        "radius": 0.0
    })";

    EXPECT_THROW(ShapeFactory::createFromJson(zeroRadiusJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateLeafThrowsOnTooSmallRadius) {
    std::string tooSmallRadiusJson = R"({
        "type": "LEAF",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}],
        "radius": 4.9
    })";  // Less than chordLength/2 (5.0)

    EXPECT_THROW(ShapeFactory::createFromJson(tooSmallRadiusJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateTriArcThrowsOnWrongVertexCount) {
    std::string wrongVertexCountJson = R"({
        "type": "TRI_ARC",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}],
        "curvatures": [-0.5, 0.0, 0.25]
    })";  // Only 2 vertices instead of 3

    EXPECT_THROW(ShapeFactory::createFromJson(wrongVertexCountJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateTriArcThrowsOnWrongCurvatureCount) {
    std::string wrongCurvatureCountJson = R"({
        "type": "TRI_ARC",
        "vertices": [{"x": 0, "y": 0}, {"x": 10, "y": 0}, {"x": 5, "y": 8}],
        "curvatures": [-0.5, 0.0]
    })";  // Only 2 curvatures instead of 3

    EXPECT_THROW(ShapeFactory::createFromJson(wrongCurvatureCountJson, mockLogger.get()),
                 std::runtime_error);
}

TEST_F(ShapeFactoryTest, CreateTriArcThrowsOnDegenerateTriangle) {
    std::string degenerateTriangleJson = R"({
        "type": "TRI_ARC",
        "vertices": [{"x": 0, "y": 0}, {"x": 5, "y": 0}, {"x": 10, "y": 0}],
        "curvatures": [-0.5, 0.0, 0.25]
    })";  // Collinear vertices

    EXPECT_THROW(ShapeFactory::createFromJson(degenerateTriangleJson, mockLogger.get()),
                 std::runtime_error);
}

// ===============================
// Edge Cases and Integration Tests
// ===============================

TEST_F(ShapeFactoryTest, CreateShapeWorksWithoutLogger) {
    std::string leafJson = R"({
        "type": "LEAF",
        "vertices": [{"x": -5, "y": -5}, {"x": 5, "y": 5}],
        "radius": 10.0
    })";

    // Should work with nullptr logger
    auto shape = ShapeFactory::createFromJson(leafJson, nullptr);

    ASSERT_NE(shape, nullptr);
    auto* leaf = dynamic_cast<Leaf*>(shape.get());
    ASSERT_NE(leaf, nullptr);
}

TEST_F(ShapeFactoryTest, CreateShapeHandlesComplexJson) {
    std::string complexJson = R"({
        "id": "shape_123",
        "type": "LEAF",
        "metadata": {
            "created": "2024-01-01",
            "author": "test"
        },
        "vertices": [{"x": 0.0, "y": 0.0}, {"x": 15.5, "y": 0.0}],
        "radius": 12.75,
        "style": {
            "color": "blue",
            "width": 2
        }
    })";

    auto shape = ShapeFactory::createFromJson(complexJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);
    auto* leaf = dynamic_cast<Leaf*>(shape.get());
    ASSERT_NE(leaf, nullptr);

    EXPECT_TRUE(leaf->getFocus1().equals(Point2D(0.0, 0.0)));
    EXPECT_TRUE(leaf->getFocus2().equals(Point2D(15.5, 0.0)));
    EXPECT_DOUBLE_EQ(leaf->getRadius(), 12.75);
}

TEST_F(ShapeFactoryTest, CreateShapeHandlesMinimalValidJson) {
    std::string minimalJson =
        R"({"type":"LEAF","vertices":[{"x":0,"y":0},{"x":2,"y":0}],"radius":1})";

    auto shape = ShapeFactory::createFromJson(minimalJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);
    auto* leaf = dynamic_cast<Leaf*>(shape.get());
    ASSERT_NE(leaf, nullptr);
}

TEST_F(ShapeFactoryTest, CreateShapeHandlesJsonWithExtraWhitespace) {
    std::string spacedJson = R"({
        "type"    :    "LEAF"   ,
        "vertices"  :  [
            {  "x"  :  0  ,  "y"  :  0  }  ,
            {  "x"  :  10  ,  "y"  :  0  }
        ]  ,
        "radius"  :  7.5
    })";

    auto shape = ShapeFactory::createFromJson(spacedJson, mockLogger.get());

    ASSERT_NE(shape, nullptr);
    auto* leaf = dynamic_cast<Leaf*>(shape.get());
    ASSERT_NE(leaf, nullptr);
    EXPECT_DOUBLE_EQ(leaf->getRadius(), 7.5);
}


/**
 * Unit tests for DesignParser
 */

#include <gtest/gtest.h>

#include "../../include/geometry/Leaf.h"
#include "../../include/geometry/TriArc.h"
#include "../../include/parsers/DesignParser.h"

using namespace ChipCarving::Parsers;
using namespace ChipCarving::Geometry;

class DesignParserTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Sample JSON for testing
        validLeafJson = R"({
            "version": "2.0",
            "metadata": {
                "name": "Test Design",
                "author": "Test Author"
            },
            "shapes": [
                {
                    "type": "LEAF",
                    "vertices": [
                        {"x": 0.0, "y": 0.0},
                        {"x": 10.0, "y": 0.0}
                    ],
                    "radius": 6.5
                }
            ]
        })";

        validTriArcJson = R"({
            "version": "2.0",
            "shapes": [
                {
                    "type": "TRI_ARC",
                    "vertices": [
                        {"x": 0.0, "y": 0.0},
                        {"x": 10.0, "y": 0.0},
                        {"x": 5.0, "y": 8.66}
                    ],
                    "curvatures": [-0.125, -0.125, -0.125]
                }
            ]
        })";

        mixedShapesJson = R"({
            "version": "2.0",
            "shapes": [
                {
                    "type": "LEAF",
                    "vertices": [
                        {"x": 0.0, "y": 0.0},
                        {"x": 10.0, "y": 0.0}
                    ],
                    "radius": 6.5
                },
                {
                    "type": "TRI_ARC",
                    "vertices": [
                        {"x": 20.0, "y": 0.0},
                        {"x": 30.0, "y": 0.0},
                        {"x": 25.0, "y": 8.66}
                    ],
                    "curvatures": [-0.1, -0.15, -0.2]
                }
            ]
        })";
    }

    std::string validLeafJson;
    std::string validTriArcJson;
    std::string mixedShapesJson;
};

TEST_F(DesignParserTest, ParseValidLeafDesign) {
    auto design = DesignParser::parseFromString(validLeafJson);

    EXPECT_EQ(design.version, "2.0");
    EXPECT_EQ(design.shapes.size(), 1);

    // Check metadata
    EXPECT_TRUE(design.metadata.name.has_value());
    EXPECT_EQ(design.metadata.name.value(), "Test Design");
    EXPECT_TRUE(design.metadata.author.has_value());
    EXPECT_EQ(design.metadata.author.value(), "Test Author");

    // Check shape
    auto* leaf = dynamic_cast<Leaf*>(design.shapes[0].get());
    EXPECT_NE(leaf, nullptr);
    EXPECT_NEAR(leaf->getFocus1().x, 0.0, 1e-9);
    EXPECT_NEAR(leaf->getFocus1().y, 0.0, 1e-9);
    EXPECT_NEAR(leaf->getFocus2().x, 10.0, 1e-9);
    EXPECT_NEAR(leaf->getFocus2().y, 0.0, 1e-9);
    EXPECT_NEAR(leaf->getRadius(), 6.5, 1e-9);
}

TEST_F(DesignParserTest, ParseValidTriArcDesign) {
    auto design = DesignParser::parseFromString(validTriArcJson);

    EXPECT_EQ(design.version, "2.0");
    EXPECT_EQ(design.shapes.size(), 1);

    // Check shape
    auto* triArc = dynamic_cast<TriArc*>(design.shapes[0].get());
    EXPECT_NE(triArc, nullptr);
    EXPECT_NEAR(triArc->getVertex(0).x, 0.0, 1e-9);
    EXPECT_NEAR(triArc->getVertex(0).y, 0.0, 1e-9);
    EXPECT_NEAR(triArc->getVertex(1).x, 10.0, 1e-9);
    EXPECT_NEAR(triArc->getVertex(1).y, 0.0, 1e-9);
    EXPECT_NEAR(triArc->getVertex(2).x, 5.0, 1e-9);
    EXPECT_NEAR(triArc->getVertex(2).y, 8.66, 1e-9);

    // Check bulge factors
    EXPECT_NEAR(triArc->getBulgeFactor(0), -0.125, 1e-9);
    EXPECT_NEAR(triArc->getBulgeFactor(1), -0.125, 1e-9);
    EXPECT_NEAR(triArc->getBulgeFactor(2), -0.125, 1e-9);
}

TEST_F(DesignParserTest, ParseMixedShapesDesign) {
    auto design = DesignParser::parseFromString(mixedShapesJson);

    EXPECT_EQ(design.version, "2.0");
    EXPECT_EQ(design.shapes.size(), 2);

    // Check first shape (Leaf)
    auto* leaf = dynamic_cast<Leaf*>(design.shapes[0].get());
    EXPECT_NE(leaf, nullptr);

    // Check second shape (TriArc)
    auto* triArc = dynamic_cast<TriArc*>(design.shapes[1].get());
    EXPECT_NE(triArc, nullptr);
    EXPECT_NEAR(triArc->getVertex(0).x, 20.0, 1e-9);
    EXPECT_NEAR(triArc->getBulgeFactor(0), -0.1, 1e-9);
    EXPECT_NEAR(triArc->getBulgeFactor(1), -0.15, 1e-9);
    EXPECT_NEAR(triArc->getBulgeFactor(2), -0.2, 1e-9);
}

TEST_F(DesignParserTest, SchemaValidation) {
    EXPECT_TRUE(DesignParser::validateSchema(validLeafJson));
    EXPECT_TRUE(DesignParser::validateSchema(validTriArcJson));
    EXPECT_TRUE(DesignParser::validateSchema(mixedShapesJson));

    // Invalid schema - wrong version
    std::string invalidVersion = R"({"version": "1.0", "shapes": []})";
    EXPECT_FALSE(DesignParser::validateSchema(invalidVersion));

    // Invalid schema - no shapes
    std::string noShapes = R"({"version": "2.0"})";
    EXPECT_FALSE(DesignParser::validateSchema(noShapes));
}

TEST_F(DesignParserTest, ErrorHandling) {
    // Invalid JSON
    EXPECT_THROW(DesignParser::parseFromString("invalid json"), std::runtime_error);

    // Missing version
    std::string noVersion = R"({"shapes": []})";
    EXPECT_THROW(DesignParser::parseFromString(noVersion), std::runtime_error);

    // Wrong version
    std::string wrongVersion = R"({"version": "1.0", "shapes": []})";
    EXPECT_THROW(DesignParser::parseFromString(wrongVersion), std::runtime_error);

    // Empty shapes array
    std::string emptyShapes = R"({"version": "2.0", "shapes": []})";
    EXPECT_THROW(DesignParser::parseFromString(emptyShapes), std::runtime_error);

    // Unknown shape type
    std::string unknownShape = R"({
        "version": "2.0",
        "shapes": [
            {
                "type": "UNKNOWN_SHAPE",
                "vertices": [{"x": 0, "y": 0}]
            }
        ]
    })";
    EXPECT_THROW(DesignParser::parseFromString(unknownShape), std::runtime_error);
}

TEST_F(DesignParserTest, LeafValidation) {
    // Invalid radius
    std::string invalidRadius = R"({
        "version": "2.0",
        "shapes": [
            {
                "type": "LEAF",
                "vertices": [
                    {"x": 0.0, "y": 0.0},
                    {"x": 10.0, "y": 0.0}
                ],
                "radius": -1.0
            }
        ]
    })";
    EXPECT_THROW(DesignParser::parseFromString(invalidRadius), std::runtime_error);

    // Too few vertices
    std::string tooFewVertices = R"({
        "version": "2.0",
        "shapes": [
            {
                "type": "LEAF",
                "vertices": [
                    {"x": 0.0, "y": 0.0}
                ],
                "radius": 5.0
            }
        ]
    })";
    EXPECT_THROW(DesignParser::parseFromString(tooFewVertices), std::runtime_error);
}

TEST_F(DesignParserTest, TriArcValidation) {
    // Too few vertices
    std::string tooFewVertices = R"({
        "version": "2.0",
        "shapes": [
            {
                "type": "TRI_ARC",
                "vertices": [
                    {"x": 0.0, "y": 0.0},
                    {"x": 10.0, "y": 0.0}
                ],
                "curvatures": [-0.125, -0.125, -0.125]
            }
        ]
    })";
    EXPECT_THROW(DesignParser::parseFromString(tooFewVertices), std::runtime_error);

    // Wrong number of curvatures
    std::string wrongCurvatures = R"({
        "version": "2.0",
        "shapes": [
            {
                "type": "TRI_ARC",
                "vertices": [
                    {"x": 0.0, "y": 0.0},
                    {"x": 10.0, "y": 0.0},
                    {"x": 5.0, "y": 8.66}
                ],
                "curvatures": [-0.125, -0.125]
            }
        ]
    })";
    EXPECT_THROW(DesignParser::parseFromString(wrongCurvatures), std::runtime_error);

    // Degenerate triangle (collinear points)
    std::string degenerateTriangle = R"({
        "version": "2.0",
        "shapes": [
            {
                "type": "TRI_ARC",
                "vertices": [
                    {"x": 0.0, "y": 0.0},
                    {"x": 5.0, "y": 0.0},
                    {"x": 10.0, "y": 0.0}
                ],
                "curvatures": [-0.125, -0.125, -0.125]
            }
        ]
    })";
    EXPECT_THROW(DesignParser::parseFromString(degenerateTriangle), std::runtime_error);
}
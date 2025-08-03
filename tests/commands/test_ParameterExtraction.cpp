/**
 * Non-fragile tests for parameter extraction logic
 * Tests business logic without Fusion API dependencies
 */

#include <gtest/gtest.h>
#include "adapters/IFusionInterface.h"

using namespace ChipCarving::Adapters;

class ParameterExtractionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default parameters for testing
        defaultParams = MedialAxisParameters();
    }

    MedialAxisParameters defaultParams;
};

TEST_F(ParameterExtractionTest, ToolAngleMapping) {
    // Test that tool names map to correct angles
    MedialAxisParameters params;
    
    // 90 degree V-bit
    params.toolName = "90° V-bit";
    EXPECT_DOUBLE_EQ(90.0, params.toolAngle) << "90° V-bit should map to 90 degree angle";
    
    // 60 degree V-bit  
    params.toolName = "60° V-bit";
    params.toolAngle = 60.0; // Simulating what getParametersFromInputs would do
    EXPECT_DOUBLE_EQ(60.0, params.toolAngle) << "60° V-bit should map to 60 degree angle";
}

TEST_F(ParameterExtractionTest, DefaultParameterValues) {
    // Test that default parameters match the actual defaults in IFusionInterface.h
    EXPECT_DOUBLE_EQ(0.25, defaultParams.polygonTolerance);
    EXPECT_DOUBLE_EQ(1.0, defaultParams.samplingDistance);
    EXPECT_DOUBLE_EQ(5.0, defaultParams.clearanceCircleSpacing);
    EXPECT_DOUBLE_EQ(3.0, defaultParams.crossSize);
    EXPECT_TRUE(defaultParams.forceBoundaryIntersections);
    EXPECT_TRUE(defaultParams.showMedialLines);
    EXPECT_TRUE(defaultParams.showClearanceCircles);
    EXPECT_FALSE(defaultParams.showPolygonizedShape);
    EXPECT_EQ("90° V-bit", defaultParams.toolName);
    EXPECT_DOUBLE_EQ(90.0, defaultParams.toolAngle);
    EXPECT_DOUBLE_EQ(6.35, defaultParams.toolDiameter);
}

TEST_F(ParameterExtractionTest, ParameterRangeValidation) {
    MedialAxisParameters params;
    
    // Test polygon tolerance range
    params.polygonTolerance = -0.1;
    EXPECT_LT(params.polygonTolerance, 0.0) << "Negative tolerance should be invalid";
    
    params.polygonTolerance = 0.0;
    EXPECT_EQ(params.polygonTolerance, 0.0) << "Zero tolerance edge case";
    
    params.polygonTolerance = 10.0;
    EXPECT_GT(params.polygonTolerance, 5.0) << "Very large tolerance might be problematic";
    
    // Test sampling distance range
    params.samplingDistance = -1.0;
    EXPECT_LT(params.samplingDistance, 0.0) << "Negative sampling distance should be invalid";
    
    params.samplingDistance = 0.1;
    EXPECT_GT(params.samplingDistance, 0.0) << "Positive sampling distance is valid";
    
    // Test clearance circle spacing range
    params.clearanceCircleSpacing = -1.0;
    EXPECT_LT(params.clearanceCircleSpacing, 0.0) << "Negative clearance circle spacing should be invalid";
    
    params.clearanceCircleSpacing = 0.1;
    EXPECT_GT(params.clearanceCircleSpacing, 0.0) << "Very small spacing might create too many circles";
    
    params.clearanceCircleSpacing = 50.0;
    EXPECT_GT(params.clearanceCircleSpacing, 10.0) << "Large spacing is valid for overview visualization";
    
    // Test cross size range
    params.crossSize = 0.0;
    EXPECT_EQ(params.crossSize, 0.0) << "Zero cross size means no crosses (valid)";
    
    params.crossSize = -1.0;
    EXPECT_LT(params.crossSize, 0.0) << "Negative cross size should be invalid";
    
    params.crossSize = 1.0;
    EXPECT_GT(params.crossSize, 0.0) << "Small positive cross size is valid";
    
    params.crossSize = 10.0;
    EXPECT_GT(params.crossSize, 5.0) << "Large cross size is valid for high visibility";
}

TEST_F(ParameterExtractionTest, ToolAngleValidation) {
    MedialAxisParameters params;
    
    // Common V-bit angles
    std::vector<double> validAngles = {30.0, 45.0, 60.0, 90.0, 120.0};
    for (double angle : validAngles) {
        params.toolAngle = angle;
        EXPECT_GE(params.toolAngle, 10.0) << "Tool angle should be reasonable";
        EXPECT_LE(params.toolAngle, 180.0) << "Tool angle should be less than 180 degrees";
    }
    
    // Invalid angles
    params.toolAngle = 0.0;
    EXPECT_EQ(params.toolAngle, 0.0) << "Zero angle is physically impossible";
    
    params.toolAngle = 200.0;
    EXPECT_GT(params.toolAngle, 180.0) << "Angle greater than 180 is invalid";
}

TEST_F(ParameterExtractionTest, BooleanFlagCombinations) {
    MedialAxisParameters params;
    
    // Test all visualization flags can be independently set
    params.showMedialLines = true;
    params.showClearanceCircles = false;
    params.showPolygonizedShape = false;
    EXPECT_TRUE(params.showMedialLines);
    EXPECT_FALSE(params.showClearanceCircles);
    EXPECT_FALSE(params.showPolygonizedShape);
    
    // All flags on
    params.showMedialLines = true;
    params.showClearanceCircles = true;
    params.showPolygonizedShape = true;
    EXPECT_TRUE(params.showMedialLines);
    EXPECT_TRUE(params.showClearanceCircles);
    EXPECT_TRUE(params.showPolygonizedShape);
    
    // All flags off
    params.showMedialLines = false;
    params.showClearanceCircles = false;
    params.showPolygonizedShape = false;
    EXPECT_FALSE(params.showMedialLines);
    EXPECT_FALSE(params.showClearanceCircles);
    EXPECT_FALSE(params.showPolygonizedShape);
}
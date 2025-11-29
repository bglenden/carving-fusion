/**
 * Unit tests for unit conversion utilities
 */

#include <gtest/gtest.h>
#include <cmath>
#include "utils/UnitConversion.h"

using namespace ChipCarving::Utils;

class UnitConversionTest : public ::testing::Test {
protected:
    // Helper to check floating point equality with tolerance
    bool isNearlyEqual(double a, double b, double epsilon = 1e-10) {
        return std::abs(a - b) < epsilon;
    }
};

// Length conversion tests
TEST_F(UnitConversionTest, FusionLengthToMm) {
    // Basic conversions
    EXPECT_DOUBLE_EQ(fusionLengthToMm(1.0), 10.0);  // 1cm = 10mm
    EXPECT_DOUBLE_EQ(fusionLengthToMm(0.1), 1.0);   // 0.1cm = 1mm
    EXPECT_DOUBLE_EQ(fusionLengthToMm(10.0), 100.0); // 10cm = 100mm
    
    // Zero
    EXPECT_DOUBLE_EQ(fusionLengthToMm(0.0), 0.0);
    
    // Negative values
    EXPECT_DOUBLE_EQ(fusionLengthToMm(-1.0), -10.0);
    EXPECT_DOUBLE_EQ(fusionLengthToMm(-0.5), -5.0);
    
    // Fractional values
    EXPECT_DOUBLE_EQ(fusionLengthToMm(0.025), 0.25);  // 0.025cm = 0.25mm
    EXPECT_DOUBLE_EQ(fusionLengthToMm(2.54), 25.4);   // 1 inch in cm to mm
}

TEST_F(UnitConversionTest, MmToFusionLength) {
    // Basic conversions
    EXPECT_DOUBLE_EQ(mmToFusionLength(10.0), 1.0);   // 10mm = 1cm
    EXPECT_DOUBLE_EQ(mmToFusionLength(1.0), 0.1);    // 1mm = 0.1cm
    EXPECT_DOUBLE_EQ(mmToFusionLength(100.0), 10.0); // 100mm = 10cm
    
    // Zero
    EXPECT_DOUBLE_EQ(mmToFusionLength(0.0), 0.0);
    
    // Negative values
    EXPECT_DOUBLE_EQ(mmToFusionLength(-10.0), -1.0);
    EXPECT_DOUBLE_EQ(mmToFusionLength(-5.0), -0.5);
    
    // Fractional values
    EXPECT_DOUBLE_EQ(mmToFusionLength(0.25), 0.025);  // 0.25mm = 0.025cm
    EXPECT_DOUBLE_EQ(mmToFusionLength(25.4), 2.54);   // 1 inch in mm to cm
}

TEST_F(UnitConversionTest, LengthRoundTripConversion) {
    // Test that converting back and forth gives original value
    const double testValues[] = {0.0, 1.0, 10.0, 0.1, 0.01, -5.0, 123.456, 0.00001};
    
    for (double value : testValues) {
        // cm -> mm -> cm
        double roundTrip1 = mmToFusionLength(fusionLengthToMm(value));
        EXPECT_TRUE(isNearlyEqual(roundTrip1, value)) 
            << "Round trip failed for " << value << ", got " << roundTrip1;
        
        // mm -> cm -> mm
        double roundTrip2 = fusionLengthToMm(mmToFusionLength(value));
        EXPECT_TRUE(isNearlyEqual(roundTrip2, value))
            << "Round trip failed for " << value << ", got " << roundTrip2;
    }
}

// Angle conversion tests
TEST_F(UnitConversionTest, FusionAngleToDegrees) {
    // Common angles
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(0.0), 0.0));
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(M_PI), 180.0));
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(M_PI / 2), 90.0));
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(M_PI / 4), 45.0));
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(2 * M_PI), 360.0));
    
    // Negative angles
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(-M_PI), -180.0));
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(-M_PI / 2), -90.0));
}

TEST_F(UnitConversionTest, DegreesToFusionAngle) {
    // Common angles
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(0.0), 0.0));
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(180.0), M_PI));
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(90.0), M_PI / 2));
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(45.0), M_PI / 4));
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(360.0), 2 * M_PI));
    
    // Negative angles
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(-180.0), -M_PI));
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(-90.0), -M_PI / 2));
}

TEST_F(UnitConversionTest, AngleRoundTripConversion) {
    // Test that converting back and forth gives original value
    const double testDegrees[] = {0.0, 45.0, 90.0, 180.0, 270.0, 360.0, -45.0, -180.0, 123.456};
    
    for (double degrees : testDegrees) {
        // degrees -> radians -> degrees
        double roundTrip1 = fusionAngleToDegrees(degreesToFusionAngle(degrees));
        EXPECT_TRUE(isNearlyEqual(roundTrip1, degrees))
            << "Round trip failed for " << degrees << " degrees, got " << roundTrip1;
    }
    
    const double testRadians[] = {0.0, M_PI/4, M_PI/2, M_PI, 3*M_PI/2, 2*M_PI, -M_PI/4, -M_PI, 2.1};
    
    for (double radians : testRadians) {
        // radians -> degrees -> radians
        double roundTrip2 = degreesToFusionAngle(fusionAngleToDegrees(radians));
        EXPECT_TRUE(isNearlyEqual(roundTrip2, radians))
            << "Round trip failed for " << radians << " radians, got " << roundTrip2;
    }
}

// Edge cases
TEST_F(UnitConversionTest, ExtremeLengthValues) {
    // Very large values
    EXPECT_DOUBLE_EQ(fusionLengthToMm(1e6), 1e7);
    EXPECT_DOUBLE_EQ(mmToFusionLength(1e7), 1e6);
    
    // Very small values
    EXPECT_TRUE(isNearlyEqual(fusionLengthToMm(1e-6), 1e-5));
    EXPECT_TRUE(isNearlyEqual(mmToFusionLength(1e-5), 1e-6));
}

TEST_F(UnitConversionTest, ExtremeAngleValues) {
    // Multiple rotations
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(10 * M_PI), 1800.0));
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(720.0), 4 * M_PI));
    
    // Very small angles
    EXPECT_TRUE(isNearlyEqual(fusionAngleToDegrees(0.001), 0.0573, 0.001));
    EXPECT_TRUE(isNearlyEqual(degreesToFusionAngle(0.001), 0.0000175, 0.0000001));
}

// Practical use cases
TEST_F(UnitConversionTest, PracticalParameterValues) {
    // Polygon tolerance: 0.25mm default
    double polygonToleranceMm = 0.25;
    double polygonToleranceCm = mmToFusionLength(polygonToleranceMm);
    EXPECT_DOUBLE_EQ(polygonToleranceCm, 0.025);
    
    // Sampling distance: 1.0mm default
    double samplingDistanceMm = 1.0;
    double samplingDistanceCm = mmToFusionLength(samplingDistanceMm);
    EXPECT_DOUBLE_EQ(samplingDistanceCm, 0.1);
    
    // Tool angles
    double angle90Deg = 90.0;
    double angle90Rad = degreesToFusionAngle(angle90Deg);
    EXPECT_TRUE(isNearlyEqual(angle90Rad, M_PI / 2));
    
    double angle60Deg = 60.0;
    double angle60Rad = degreesToFusionAngle(angle60Deg);
    EXPECT_TRUE(isNearlyEqual(angle60Rad, M_PI / 3));
}
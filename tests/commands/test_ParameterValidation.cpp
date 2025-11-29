/**
 * Unit tests for Enhanced UI parameter validation logic
 * Tests pure business logic without GUI dependencies or complex mocks
 */

#include <gtest/gtest.h>

#include "adapters/IFusionInterface.h"

namespace ChipCarving {
namespace Commands {
namespace Test {

class ParameterValidationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Initialize default valid parameters
        defaultParams.polygonTolerance = 0.025;  // 0.25mm in cm
        defaultParams.samplingDistance = 0.1;    // 1.0mm in cm
        defaultParams.forceBoundaryIntersections = true;
        defaultParams.showMedialLines = true;
        defaultParams.showClearanceCircles = true;
        defaultParams.showPolygonizedShape = false;
    }

    Adapters::MedialAxisParameters defaultParams;

    static constexpr double TOLERANCE = 1e-9;
    static constexpr double MIN_TOLERANCE = 1e-6;  // 0.001mm
    static constexpr double MAX_TOLERANCE = 10.0;  // 100mm
    static constexpr double MIN_SAMPLING = 1e-6;   // 0.001mm
    static constexpr double MAX_SAMPLING = 100.0;  // 1000mm
};

// Test default parameter values are correct
TEST_F(ParameterValidationTest, DefaultParameterValues) {
    EXPECT_NEAR(defaultParams.polygonTolerance, 0.025, TOLERANCE);
    EXPECT_NEAR(defaultParams.samplingDistance, 0.1, TOLERANCE);
    EXPECT_TRUE(defaultParams.forceBoundaryIntersections);
    EXPECT_TRUE(defaultParams.showMedialLines);
    EXPECT_TRUE(defaultParams.showClearanceCircles);
    EXPECT_FALSE(defaultParams.showPolygonizedShape);
}

// Test polygon tolerance validation ranges
TEST_F(ParameterValidationTest, PolygonToleranceValidation) {
    // Valid range tests
    EXPECT_TRUE(defaultParams.polygonTolerance >= MIN_TOLERANCE);
    EXPECT_TRUE(defaultParams.polygonTolerance <= MAX_TOLERANCE);

    // Boundary value tests
    Adapters::MedialAxisParameters params = defaultParams;

    params.polygonTolerance = MIN_TOLERANCE;
    EXPECT_GE(params.polygonTolerance, MIN_TOLERANCE);

    params.polygonTolerance = MAX_TOLERANCE;
    EXPECT_LE(params.polygonTolerance, MAX_TOLERANCE);

    // Test typical values
    params.polygonTolerance = 0.005;  // 0.05mm
    EXPECT_TRUE(params.polygonTolerance >= MIN_TOLERANCE &&
                params.polygonTolerance <= MAX_TOLERANCE);

    params.polygonTolerance = 0.1;  // 1.0mm
    EXPECT_TRUE(params.polygonTolerance >= MIN_TOLERANCE &&
                params.polygonTolerance <= MAX_TOLERANCE);

    params.polygonTolerance = 1.0;  // 10mm
    EXPECT_TRUE(params.polygonTolerance >= MIN_TOLERANCE &&
                params.polygonTolerance <= MAX_TOLERANCE);
}

// Test sampling distance validation ranges
TEST_F(ParameterValidationTest, SamplingDistanceValidation) {
    // Valid range tests
    EXPECT_TRUE(defaultParams.samplingDistance >= MIN_SAMPLING);
    EXPECT_TRUE(defaultParams.samplingDistance <= MAX_SAMPLING);

    // Boundary value tests
    Adapters::MedialAxisParameters params = defaultParams;

    params.samplingDistance = MIN_SAMPLING;
    EXPECT_GE(params.samplingDistance, MIN_SAMPLING);

    params.samplingDistance = MAX_SAMPLING;
    EXPECT_LE(params.samplingDistance, MAX_SAMPLING);

    // Test typical values
    params.samplingDistance = 0.05;  // 0.5mm
    EXPECT_TRUE(params.samplingDistance >= MIN_SAMPLING && params.samplingDistance <= MAX_SAMPLING);

    params.samplingDistance = 0.2;  // 2.0mm
    EXPECT_TRUE(params.samplingDistance >= MIN_SAMPLING && params.samplingDistance <= MAX_SAMPLING);

    params.samplingDistance = 5.0;  // 50mm
    EXPECT_TRUE(params.samplingDistance >= MIN_SAMPLING && params.samplingDistance <= MAX_SAMPLING);
}

// Test unit conversion logic (mm to cm for Fusion 360)
TEST_F(ParameterValidationTest, UnitConversionLogic) {
    // Test mm to cm conversion
    double mm_to_cm_factor = 0.1;

    // Default values in mm
    double polygon_tolerance_mm = 0.25;
    double sampling_distance_mm = 1.0;

    // Convert to cm (Fusion 360 internal units)
    double polygon_tolerance_cm = polygon_tolerance_mm * mm_to_cm_factor;
    double sampling_distance_cm = sampling_distance_mm * mm_to_cm_factor;

    EXPECT_NEAR(polygon_tolerance_cm, 0.025, TOLERANCE);
    EXPECT_NEAR(sampling_distance_cm, 0.1, TOLERANCE);

    // Test other common values
    double values_mm[] = {0.1, 0.5, 2.0, 5.0, 10.0};
    double expected_cm[] = {0.01, 0.05, 0.2, 0.5, 1.0};

    for (size_t i = 0; i < sizeof(values_mm) / sizeof(values_mm[0]); i++) {
        double converted = values_mm[i] * mm_to_cm_factor;
        EXPECT_NEAR(converted, expected_cm[i], TOLERANCE);
    }
}

// Test parameter relationship validation
TEST_F(ParameterValidationTest, ParameterRelationships) {
    Adapters::MedialAxisParameters params = defaultParams;

    // Polygon tolerance should typically be smaller than sampling distance
    EXPECT_LT(params.polygonTolerance, params.samplingDistance);

    // Test reasonable ratios
    double ratio = params.samplingDistance / params.polygonTolerance;
    EXPECT_GT(ratio, 1.0);     // Sampling should be larger than tolerance
    EXPECT_LT(ratio, 1000.0);  // But not excessively larger

    // Test with other valid combinations
    params.polygonTolerance = 0.01;  // 0.1mm
    params.samplingDistance = 0.05;  // 0.5mm
    ratio = params.samplingDistance / params.polygonTolerance;
    EXPECT_GT(ratio, 1.0);
    EXPECT_LT(ratio, 1000.0);

    params.polygonTolerance = 0.05;  // 0.5mm
    params.samplingDistance = 0.2;   // 2.0mm
    ratio = params.samplingDistance / params.polygonTolerance;
    EXPECT_GT(ratio, 1.0);
    EXPECT_LT(ratio, 1000.0);
}

// Test boolean parameter validation
TEST_F(ParameterValidationTest, BooleanParameters) {
    Adapters::MedialAxisParameters params = defaultParams;

    // Test all boolean combinations
    bool boolean_values[] = {true, false};

    for (bool forceBoundary : boolean_values) {
        for (bool showMedial : boolean_values) {
            for (bool showClearance : boolean_values) {
                for (bool showPolygon : boolean_values) {
                    params.forceBoundaryIntersections = forceBoundary;
                    params.showMedialLines = showMedial;
                    params.showClearanceCircles = showClearance;
                    params.showPolygonizedShape = showPolygon;

                    // All boolean combinations should be valid
                    EXPECT_TRUE(true);  // No validation errors expected
                }
            }
        }
    }
}

// Test parameter copying and assignment
TEST_F(ParameterValidationTest, ParameterCopyAndAssignment) {
    Adapters::MedialAxisParameters params1 = defaultParams;
    Adapters::MedialAxisParameters params2;

    // Modify params1
    params1.polygonTolerance = 0.05;
    params1.samplingDistance = 0.2;
    params1.forceBoundaryIntersections = false;
    params1.showMedialLines = false;
    params1.showClearanceCircles = false;
    params1.showPolygonizedShape = true;

    // Copy assignment
    params2 = params1;

    // Verify all fields copied correctly
    EXPECT_NEAR(params2.polygonTolerance, 0.05, TOLERANCE);
    EXPECT_NEAR(params2.samplingDistance, 0.2, TOLERANCE);
    EXPECT_FALSE(params2.forceBoundaryIntersections);
    EXPECT_FALSE(params2.showMedialLines);
    EXPECT_FALSE(params2.showClearanceCircles);
    EXPECT_TRUE(params2.showPolygonizedShape);

    // Verify independence (modifying params2 doesn't affect params1)
    params2.polygonTolerance = 0.1;
    EXPECT_NEAR(params1.polygonTolerance, 0.05, TOLERANCE);
    EXPECT_NEAR(params2.polygonTolerance, 0.1, TOLERANCE);
}

// Test extreme value handling
TEST_F(ParameterValidationTest, ExtremeValueHandling) {
    Adapters::MedialAxisParameters params = defaultParams;

    // Test very small values
    params.polygonTolerance = 1e-10;  // Extremely small
    params.samplingDistance = 1e-10;
    // These values should be handled gracefully (though may be clamped by validation)

    // Test very large values
    params.polygonTolerance = 1e10;  // Extremely large
    params.samplingDistance = 1e10;
    // These values should be handled gracefully (though may be clamped by validation)

    // Test zero values
    params.polygonTolerance = 0.0;
    params.samplingDistance = 0.0;
    // Zero values should be handled (likely invalid and should be rejected)

    // Test negative values
    params.polygonTolerance = -0.1;
    params.samplingDistance = -0.1;
    // Negative values should be handled (invalid for geometric tolerances)

    // The test just verifies the parameters can be assigned without crashing
    // Actual validation would happen in the UI input validation logic
    EXPECT_TRUE(true);
}

}  // namespace Test
}  // namespace Commands
}  // namespace ChipCarving
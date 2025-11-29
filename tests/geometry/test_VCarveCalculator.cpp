/**
 * Unit tests for VCarveCalculator
 * Tests V-carve depth calculations, path generation, and optimization
 */

#include <gtest/gtest.h>
#include <cmath>
#include "geometry/VCarveCalculator.h"
#include "geometry/MedialAxisProcessor.h"
#include "geometry/MedialAxisUtilities.h"

using namespace ChipCarving::Geometry;
using namespace ChipCarving::Adapters;

class VCarveCalculatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        calculator = std::make_unique<VCarveCalculator>();
        
        // Set up standard parameters
        params.toolAngle = 90.0;           // 90-degree V-bit
        params.maxVCarveDepth = 10.0;      // 10mm max depth
        params.samplingDistance = 1.0;     // 1mm sampling
        params.generateVCarveToolpaths = true;
        // Surface projection is always enabled now
    }
    
    std::unique_ptr<VCarveCalculator> calculator;
    MedialAxisParameters params;
};

// Depth Calculation Tests
TEST_F(VCarveCalculatorTest, CalculateVCarveDepthBasic) {
    // Test basic depth calculation for 90-degree V-bit
    double clearanceRadius = 2.0;  // 2mm clearance
    double toolAngle = 90.0;       // 90-degree tool
    double maxDepth = 10.0;        // 10mm max depth
    
    double depth = calculator->calculateVCarveDepth(clearanceRadius, toolAngle, maxDepth);
    
    // For 90-degree tool: depth = radius / tan(45°) = radius / 1 = radius
    EXPECT_NEAR(depth, 2.0, 0.001);
}

TEST_F(VCarveCalculatorTest, CalculateVCarveDepth60Degree) {
    // Test depth calculation for 60-degree V-bit
    double clearanceRadius = 2.0;  // 2mm clearance
    double toolAngle = 60.0;       // 60-degree tool
    double maxDepth = 10.0;        // 10mm max depth
    
    double depth = calculator->calculateVCarveDepth(clearanceRadius, toolAngle, maxDepth);
    
    // For 60-degree tool: depth = radius / tan(30°) = radius / (1/√3) = radius * √3
    double expected = 2.0 * std::sqrt(3.0);
    EXPECT_NEAR(depth, expected, 0.001);
}

TEST_F(VCarveCalculatorTest, CalculateVCarveDepth120Degree) {
    // Test depth calculation for 120-degree V-bit
    double clearanceRadius = 2.0;  // 2mm clearance
    double toolAngle = 120.0;      // 120-degree tool
    double maxDepth = 10.0;        // 10mm max depth
    
    double depth = calculator->calculateVCarveDepth(clearanceRadius, toolAngle, maxDepth);
    
    // For 120-degree tool: depth = radius / tan(60°) = radius / √3
    double expected = 2.0 / std::sqrt(3.0);
    EXPECT_NEAR(depth, expected, 0.001);
}

TEST_F(VCarveCalculatorTest, CalculateVCarveDepthMaxDepthLimit) {
    // Test depth calculation with max depth limiting
    double clearanceRadius = 20.0; // Large clearance that would exceed max
    double toolAngle = 90.0;
    double maxDepth = 5.0;         // Low max depth limit
    
    double depth = calculator->calculateVCarveDepth(clearanceRadius, toolAngle, maxDepth);
    
    // Should be limited to max depth
    EXPECT_EQ(depth, 5.0);
}

TEST_F(VCarveCalculatorTest, CalculateVCarveDepthZeroClearance) {
    // Test with zero clearance (sharp corners)
    double clearanceRadius = 0.0;
    double toolAngle = 90.0;
    double maxDepth = 10.0;
    
    double depth = calculator->calculateVCarveDepth(clearanceRadius, toolAngle, maxDepth);
    
    // Zero clearance should give zero depth
    EXPECT_EQ(depth, 0.0);
}

TEST_F(VCarveCalculatorTest, CalculateVCarveDepthInvalidInputs) {
    // Test with invalid inputs
    EXPECT_EQ(calculator->calculateVCarveDepth(-1.0, 90.0, 10.0), 0.0);    // Negative clearance
    EXPECT_EQ(calculator->calculateVCarveDepth(2.0, 0.0, 10.0), 0.0);      // Zero angle
    EXPECT_EQ(calculator->calculateVCarveDepth(2.0, 180.0, 10.0), 0.0);    // 180-degree angle
    EXPECT_EQ(calculator->calculateVCarveDepth(2.0, -30.0, 10.0), 0.0);    // Negative angle
    EXPECT_EQ(calculator->calculateVCarveDepth(2.0, 200.0, 10.0), 0.0);    // > 180-degree angle
}

// Parameter Validation Tests (via public interface)
TEST_F(VCarveCalculatorTest, GenerateVCarvePathsValidatesParameters) {
    // Test parameter validation through the public interface
    MedialAxisResults medialResults;
    medialResults.success = true;
    
    // Add valid medial data
    std::vector<Point2D> chain;
    std::vector<double> clearances;
    chain.emplace_back(Point2D(0, 0));
    clearances.push_back(2.0);
    medialResults.chains.push_back(chain);
    medialResults.clearanceRadii.push_back(clearances);
    
    // Test with invalid tool angle
    MedialAxisParameters invalidParams = params;
    invalidParams.toolAngle = 0.0;  // Invalid
    
    VCarveResults results = calculator->generateVCarvePaths(medialResults, invalidParams);
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());
    
    // Test with invalid max depth
    invalidParams = params;
    invalidParams.maxVCarveDepth = -1.0;  // Invalid
    
    results = calculator->generateVCarvePaths(medialResults, invalidParams);
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());
    
    // Test with invalid sampling distance
    invalidParams = params;
    invalidParams.samplingDistance = 0.0;  // Invalid
    
    results = calculator->generateVCarvePaths(medialResults, invalidParams);
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());
}

// Integration Tests
TEST_F(VCarveCalculatorTest, GenerateVCarvePathsFromMedialResults) {
    // Create mock medial axis results
    MedialAxisResults medialResults;
    medialResults.success = true;
    
    // Add a simple chain
    std::vector<Point2D> chain;
    std::vector<double> clearances;
    chain.emplace_back(Point2D(0, 0));
    chain.emplace_back(Point2D(10, 0));
    chain.emplace_back(Point2D(20, 0));
    clearances.push_back(2.0);
    clearances.push_back(1.5);
    clearances.push_back(1.0);
    medialResults.chains.push_back(chain);
    medialResults.clearanceRadii.push_back(clearances);
    
    VCarveResults results = calculator->generateVCarvePaths(medialResults, params);
    
    EXPECT_TRUE(results.success);
    EXPECT_GT(results.totalPaths, 0);
    EXPECT_GT(results.totalPoints, 0);
    EXPECT_GT(results.totalLength, 0.0);
    EXPECT_TRUE(results.errorMessage.empty());
}

TEST_F(VCarveCalculatorTest, GenerateVCarvePathsInvalidMedialResults) {
    MedialAxisResults invalidResults;
    invalidResults.success = false;
    
    VCarveResults results = calculator->generateVCarvePaths(invalidResults, params);
    
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());
}

TEST_F(VCarveCalculatorTest, GenerateVCarvePathsInvalidParameters) {
    MedialAxisResults medialResults;
    medialResults.success = true;
    
    MedialAxisParameters invalidParams = params;
    invalidParams.toolAngle = 0.0;  // Invalid angle
    
    VCarveResults results = calculator->generateVCarvePaths(medialResults, invalidParams);
    
    EXPECT_FALSE(results.success);
    EXPECT_FALSE(results.errorMessage.empty());
}

// Real-world scenario tests
TEST_F(VCarveCalculatorTest, RealWorldTriangularShape) {
    // Simulate a triangular medial axis result
    MedialAxisResults medialResults;
    medialResults.success = true;
    
    // Create a path that goes from corner (small clearance) to center (large clearance)
    // Note: MedialAxisResults stores coordinates and clearances in cm, not mm
    std::vector<Point2D> chain;
    std::vector<double> clearances;
    chain.emplace_back(Point2D(0, 0));      // Corner (cm)
    chain.emplace_back(Point2D(0.2, 0.1));  // Partway (cm)
    chain.emplace_back(Point2D(0.5, 0.25)); // Center (cm)
    chain.emplace_back(Point2D(0.8, 0.1));  // Partway (cm)
    chain.emplace_back(Point2D(1.0, 0));    // Corner (cm)
    clearances.push_back(0.01);             // Corner (0.1mm = 0.01cm)
    clearances.push_back(0.1);              // Partway (1.0mm = 0.1cm)
    clearances.push_back(0.25);             // Center (2.5mm = 0.25cm)
    clearances.push_back(0.1);              // Partway (1.0mm = 0.1cm)
    clearances.push_back(0.01);             // Corner (0.1mm = 0.01cm)
    medialResults.chains.push_back(chain);
    medialResults.clearanceRadii.push_back(clearances);
    
    VCarveResults results = calculator->generateVCarvePaths(medialResults, params);
    
    EXPECT_TRUE(results.success);
    EXPECT_EQ(results.totalPaths, 1);
    EXPECT_GT(results.totalPoints, 0);  // Should have points (exact count depends on sampling)
    
    // Check that depths vary appropriately (deeper in center, shallow at corners)
    EXPECT_GT(results.maxDepth, results.minDepth);
    EXPECT_NEAR(results.maxDepth, 2.5, 0.1);  // Should be around the center clearance
    EXPECT_NEAR(results.minDepth, 0.1, 0.1);  // Should be around the corner clearance
}

/**
 * Test that VCarveCalculator uses raw medial axis data without additional sampling
 * Validates that double interpolation removal is working correctly
 */
TEST_F(VCarveCalculatorTest, UsesRawMedialAxisData) {
    // Create MedialAxisResults with known, controlled point count
    MedialAxisResults medialResults;
    medialResults.success = true;
    medialResults.numChains = 1;
    medialResults.totalPoints = 7;  // Exact known count
    
    // Single chain with exactly 7 points (no sampling, raw OpenVoronoi data)
    std::vector<Point2D> chain;
    std::vector<double> clearances;
    
    chain.emplace_back(Point2D(0.0, 0.0));   clearances.push_back(0.05);  // Point 1
    chain.emplace_back(Point2D(1.0, 0.0));   clearances.push_back(0.15);  // Point 2
    chain.emplace_back(Point2D(2.0, 0.5));   clearances.push_back(0.25);  // Point 3
    chain.emplace_back(Point2D(3.0, 1.0));   clearances.push_back(0.30);  // Point 4
    chain.emplace_back(Point2D(4.0, 0.5));   clearances.push_back(0.20);  // Point 5
    chain.emplace_back(Point2D(5.0, 0.0));   clearances.push_back(0.10);  // Point 6
    chain.emplace_back(Point2D(6.0, 0.0));   clearances.push_back(0.05);  // Point 7
    
    medialResults.chains.push_back(chain);
    medialResults.clearanceRadii.push_back(clearances);
    
    // Configure parameters for direct processing (no additional sampling)
    MedialAxisParameters params;
    params.toolAngle = 90.0;
    params.maxVCarveDepth = 10.0;
    params.samplingDistance = 0.5;  // Should not affect raw data usage
    params.generateVCarveToolpaths = true;
    // Surface projection is always enabled now
    
    // Generate V-carve paths
    VCarveResults results = calculator->generateVCarvePaths(medialResults, params);
    
    EXPECT_TRUE(results.success) << "V-carve generation failed: " << results.errorMessage;
    EXPECT_EQ(results.totalPaths, 1) << "Should generate exactly 1 path for 1 medial axis chain";
    
    // Key test: Output should preserve the input point count (no additional interpolation)
    // VCarveCalculator should use the raw 7 medial axis points directly
    EXPECT_EQ(results.totalPoints, 7) 
        << "Expected 7 points (raw medial axis data), got " << results.totalPoints
        << " (indicates additional interpolation occurred)";
    
    // Verify the transformation maintains point-to-point correspondence
    // Each input medial axis point should correspond to exactly one V-carve point
    if (!results.paths.empty() && !results.paths[0].points.empty()) {
        EXPECT_EQ(results.paths[0].points.size(), 7)
            << "First path should have exactly 7 points matching medial axis input";
            
        // Verify depths are calculated correctly from clearances (90-degree tool)
        // depth = clearance_radius / tan(45°) = clearance_radius
        // Note: VCarveCalculator converts cm clearances to mm depths automatically
        for (size_t i = 0; i < std::min(results.paths[0].points.size(), clearances.size()); ++i) {
            double expectedDepth = clearances[i] * 10.0;  // Convert cm to mm for 90-degree tool
            double actualDepth = results.paths[0].points[i].depth;
            EXPECT_NEAR(actualDepth, expectedDepth, 0.1) 
                << "Point " << i << " depth mismatch: expected " << expectedDepth 
                << ", got " << actualDepth;
        }
    }
    
    // Verify no sampling interpolation artifacts (depths in mm)
    EXPECT_NEAR(results.minDepth, 0.5, 0.1) << "Minimum depth should match smallest clearance (in mm)";
    EXPECT_NEAR(results.maxDepth, 3.0, 0.1) << "Maximum depth should match largest clearance (in mm)";
}
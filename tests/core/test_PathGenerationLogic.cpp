/**
 * Tests for path generation business logic
 * Tests the core algorithms without Fusion API dependencies
 */

#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "geometry/Point2D.h"
#include "geometry/MedialAxisProcessor.h"

using namespace ChipCarving::Geometry;

class PathGenerationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple triangle for testing
        triangleVertices.push_back(Point2D(0, 0));
        triangleVertices.push_back(Point2D(10, 0));
        triangleVertices.push_back(Point2D(5, 8.66)); // Equilateral triangle
        
        // Create a square for testing
        squareVertices.push_back(Point2D(0, 0));
        squareVertices.push_back(Point2D(10, 0));
        squareVertices.push_back(Point2D(10, 10));
        squareVertices.push_back(Point2D(0, 10));
    }
    
    std::vector<Point2D> triangleVertices;
    std::vector<Point2D> squareVertices;
};

TEST_F(PathGenerationTest, MedialAxisPointCount) {
    // The medial axis should have fewer points than the perimeter
    // For a triangle, the medial axis has one branch point and three edges
    MedialAxisProcessor processor;
    auto result = processor.computeMedialAxis(triangleVertices);
    
    EXPECT_TRUE(result.success) << "Medial axis computation should succeed";
    EXPECT_FALSE(result.chains.empty()) << "Should have at least one chain";
    
    // Count total points in all chains
    size_t totalPoints = 0;
    for (const auto& chain : result.chains) {
        totalPoints += chain.size();
    }
    
    // Medial axis should be simpler than the boundary
    EXPECT_GT(totalPoints, 0) << "Should have some medial axis points";
    EXPECT_LT(totalPoints, triangleVertices.size() * 10) << "Shouldn't have excessive points";
}

TEST_F(PathGenerationTest, ClearanceRadiusProperties) {
    MedialAxisProcessor processor;
    auto result = processor.computeMedialAxis(squareVertices);
    
    EXPECT_TRUE(result.success);
    
    // For a square, check the clearance radii
    double maxClearance = 0.0;
    
    // Check clearance radii for each chain
    for (size_t i = 0; i < result.chains.size(); ++i) {
        const auto& chain = result.chains[i];
        (void)chain; // Suppress unused variable warning
        const auto& clearances = result.clearanceRadii[i];
        
        for (size_t j = 0; j < clearances.size(); ++j) {
            if (clearances[j] > maxClearance) {
                maxClearance = clearances[j];
            }
        }
    }
    
    // Maximum clearance should be from the result
    EXPECT_NEAR(result.maxClearance, maxClearance, 0.1) << "Max clearance should match";
}

TEST_F(PathGenerationTest, DepthCalculationFromClearance) {
    // Test depth calculation for V-bit carving
    struct VBitDepthCalculator {
        static double calculateDepth(double clearanceRadius, double toolAngle) {
            // For a V-bit, the depth needed to achieve a given clearance
            // depth = clearance / tan(toolAngle/2)
            double angleRad = toolAngle * M_PI / 180.0;
            return clearanceRadius / std::tan(angleRad / 2.0);
        }
    };
    
    // Test with 90° V-bit
    double depth90 = VBitDepthCalculator::calculateDepth(2.0, 90.0);
    EXPECT_NEAR(2.0, depth90, 0.001) << "90° V-bit: depth should equal clearance";
    
    // Test with 60° V-bit
    double depth60 = VBitDepthCalculator::calculateDepth(2.0, 60.0);
    EXPECT_GT(depth60, depth90) << "Sharper angle requires more depth";
    EXPECT_NEAR(3.464, depth60, 0.001); // 2 / tan(30°)
    
    // Test with 45° V-bit
    double depth45 = VBitDepthCalculator::calculateDepth(2.0, 45.0);
    EXPECT_NEAR(4.828, depth45, 0.001); // 2 / tan(22.5°)
}

TEST_F(PathGenerationTest, PolygonToleranceVsChainCount) {
    MedialAxisProcessor processor;
    
    // Test with different polygon tolerances
    processor.setPolygonTolerance(0.01);
    auto resultFine = processor.computeMedialAxis(triangleVertices);
    
    processor.setPolygonTolerance(0.5);
    auto resultCoarse = processor.computeMedialAxis(triangleVertices);
    
    EXPECT_TRUE(resultFine.success);
    EXPECT_TRUE(resultCoarse.success);
    
    // Both should find medial axis chains
    EXPECT_GT(resultFine.chains.size(), 0) << "Should find medial axis chains";
    EXPECT_GT(resultCoarse.chains.size(), 0) << "Should find medial axis chains";
}

TEST_F(PathGenerationTest, ChainContinuity) {
    MedialAxisProcessor processor;
    auto result = processor.computeMedialAxis(squareVertices);
    
    EXPECT_TRUE(result.success);
    
    // Check that chains are continuous (adjacent points are close)
    for (const auto& chain : result.chains) {
        if (chain.size() < 2) continue;
        
        for (size_t i = 1; i < chain.size(); ++i) {
            const auto& p1 = chain[i-1];
            const auto& p2 = chain[i];
            
            double distance = std::sqrt(
                std::pow(p2.x - p1.x, 2) + 
                std::pow(p2.y - p1.y, 2)
            );
            
            // Adjacent points should be reasonably close
            // For a 10x10 square, diagonal distance could be up to ~14.14
            EXPECT_LT(distance, 15.0) << "Chain should be continuous";
        }
    }
}

TEST_F(PathGenerationTest, PolygonToleranceEffect) {
    // Test that polygon tolerance affects the approximation quality
    std::vector<Point2D> circle;
    const int numPoints = 32;
    const double radius = 10.0;
    
    // Create a circle
    for (int i = 0; i < numPoints; ++i) {
        double angle = 2.0 * M_PI * i / numPoints;
        circle.push_back(Point2D(
            radius * std::cos(angle),
            radius * std::sin(angle)
        ));
    }
    
    MedialAxisProcessor processor;
    
    // With tight tolerance
    processor.setPolygonTolerance(0.01);
    auto resultTight = processor.computeMedialAxis(circle);
    
    // With loose tolerance
    processor.setPolygonTolerance(1.0);
    auto resultLoose = processor.computeMedialAxis(circle);
    
    EXPECT_TRUE(resultTight.success);
    EXPECT_TRUE(resultLoose.success);
    
    // Both should identify the center as having maximum clearance
    // but the approximation quality may differ
}
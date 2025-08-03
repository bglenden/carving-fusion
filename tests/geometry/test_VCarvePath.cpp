/**
 * Unit tests for VCarvePath data structures
 * Tests the core V-carve point and path representations
 */

#include <gtest/gtest.h>
#include "../../include/geometry/VCarvePath.h"

using namespace ChipCarving::Geometry;

class VCarvePathTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test data
    }
};

// VCarvePoint Tests
TEST_F(VCarvePathTest, VCarvePointConstruction) {
    Point2D pos(10.0, 20.0);
    double depth = 2.5;
    double clearance = 5.0;
    
    VCarvePoint point(pos, depth, clearance);
    
    EXPECT_EQ(point.position.x, 10.0);
    EXPECT_EQ(point.position.y, 20.0);
    EXPECT_EQ(point.depth, 2.5);
    EXPECT_EQ(point.clearanceRadius, 5.0);
}

TEST_F(VCarvePathTest, VCarvePointDefaultConstruction) {
    VCarvePoint point;
    
    EXPECT_EQ(point.position.x, 0.0);
    EXPECT_EQ(point.position.y, 0.0);
    EXPECT_EQ(point.depth, 0.0);
    EXPECT_EQ(point.clearanceRadius, 0.0);
}

// VCarvePath Tests
TEST_F(VCarvePathTest, VCarvePathDefaultConstruction) {
    VCarvePath path;
    
    EXPECT_TRUE(path.points.empty());
    EXPECT_EQ(path.totalLength, 0.0);
    EXPECT_FALSE(path.isClosed);
    EXPECT_FALSE(path.isValid());
}

TEST_F(VCarvePathTest, VCarvePathIsValidWithTwoPoints) {
    VCarvePath path;
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    path.points.emplace_back(Point2D(10, 0), 1.5, 1.8);
    
    EXPECT_TRUE(path.isValid());
}

TEST_F(VCarvePathTest, VCarvePathIsInvalidWithOnePoint) {
    VCarvePath path;
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    
    EXPECT_FALSE(path.isValid());
}

TEST_F(VCarvePathTest, VCarvePathCalculateLength) {
    VCarvePath path;
    // Create a simple rectangular path: (0,0) -> (10,0) -> (10,5) -> (0,5)
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    path.points.emplace_back(Point2D(10, 0), 1.0, 2.0);
    path.points.emplace_back(Point2D(10, 5), 1.0, 2.0);
    path.points.emplace_back(Point2D(0, 5), 1.0, 2.0);
    
    double length = path.calculateLength();
    double expectedLength = 10.0 + 5.0 + 10.0; // Three segments
    EXPECT_NEAR(length, expectedLength, 0.001);
}

TEST_F(VCarvePathTest, VCarvePathCalculateLengthEmpty) {
    VCarvePath path;
    double length = path.calculateLength();
    EXPECT_EQ(length, 0.0);
}

TEST_F(VCarvePathTest, VCarvePathCalculateLengthSinglePoint) {
    VCarvePath path;
    path.points.emplace_back(Point2D(5, 5), 1.0, 2.0);
    
    double length = path.calculateLength();
    EXPECT_EQ(length, 0.0);
}

TEST_F(VCarvePathTest, VCarvePathGetMaxDepth) {
    VCarvePath path;
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    path.points.emplace_back(Point2D(10, 0), 2.5, 1.8);
    path.points.emplace_back(Point2D(10, 5), 1.8, 1.5);
    path.points.emplace_back(Point2D(0, 5), 3.2, 1.2);
    
    double maxDepth = path.getMaxDepth();
    EXPECT_EQ(maxDepth, 3.2);
}

TEST_F(VCarvePathTest, VCarvePathGetMinDepth) {
    VCarvePath path;
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    path.points.emplace_back(Point2D(10, 0), 2.5, 1.8);
    path.points.emplace_back(Point2D(10, 5), 1.8, 1.5);
    path.points.emplace_back(Point2D(0, 5), 3.2, 1.2);
    
    double minDepth = path.getMinDepth();
    EXPECT_EQ(minDepth, 1.0);
}

TEST_F(VCarvePathTest, VCarvePathDepthsEmptyPath) {
    VCarvePath path;
    
    double maxDepth = path.getMaxDepth();
    double minDepth = path.getMinDepth();
    
    EXPECT_EQ(maxDepth, 0.0);
    EXPECT_EQ(minDepth, 0.0);
}

// VCarveResults Tests
TEST_F(VCarvePathTest, VCarveResultsDefaultConstruction) {
    VCarveResults results;
    
    EXPECT_TRUE(results.paths.empty());
    EXPECT_EQ(results.totalPaths, 0);
    EXPECT_EQ(results.totalPoints, 0);
    EXPECT_EQ(results.totalLength, 0.0);
    EXPECT_EQ(results.maxDepth, 0.0);
    EXPECT_EQ(results.minDepth, 0.0);
    EXPECT_FALSE(results.success);
    EXPECT_TRUE(results.errorMessage.empty());
}

TEST_F(VCarvePathTest, VCarveResultsUpdateStatistics) {
    VCarveResults results;
    
    // Add first path
    VCarvePath path1;
    path1.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    path1.points.emplace_back(Point2D(10, 0), 2.0, 1.8);
    path1.totalLength = 10.0;
    results.paths.push_back(path1);
    
    // Add second path
    VCarvePath path2;
    path2.points.emplace_back(Point2D(0, 5), 0.5, 1.5);
    path2.points.emplace_back(Point2D(5, 5), 3.0, 1.2);
    path2.points.emplace_back(Point2D(10, 5), 1.5, 1.0);
    path2.totalLength = 10.0;
    results.paths.push_back(path2);
    
    results.updateStatistics();
    
    EXPECT_EQ(results.totalPaths, 2);
    EXPECT_EQ(results.totalPoints, 5);  // 2 + 3 points
    EXPECT_EQ(results.totalLength, 20.0);  // 10.0 + 10.0
    EXPECT_EQ(results.maxDepth, 3.0);
    EXPECT_EQ(results.minDepth, 0.5);
}

TEST_F(VCarvePathTest, VCarveResultsUpdateStatisticsEmpty) {
    VCarveResults results;
    results.updateStatistics();
    
    EXPECT_EQ(results.totalPaths, 0);
    EXPECT_EQ(results.totalPoints, 0);
    EXPECT_EQ(results.totalLength, 0.0);
    EXPECT_EQ(results.maxDepth, 0.0);
    EXPECT_EQ(results.minDepth, 0.0);
}

TEST_F(VCarvePathTest, VCarveResultsGetSummary) {
    VCarveResults results;
    
    // Add test path
    VCarvePath path;
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    path.points.emplace_back(Point2D(10, 0), 2.0, 1.8);
    path.totalLength = 10.0;
    results.paths.push_back(path);
    
    results.updateStatistics();
    results.success = true;
    
    std::string summary = results.getSummary();
    
    // Should contain key statistics
    EXPECT_NE(summary.find("1"), std::string::npos);      // Total paths
    EXPECT_NE(summary.find("2"), std::string::npos);      // Total points
    EXPECT_NE(summary.find("10"), std::string::npos);     // Total length
}

// Real-world scenario tests
TEST_F(VCarvePathTest, RealWorldTriangularVCarve) {
    // Simulate a triangular V-carve path with varying depths
    VCarvePath path;
    
    // Triangle vertices with decreasing clearance toward center
    path.points.emplace_back(Point2D(0, 0), 0.1, 5.0);      // Corner, shallow cut
    path.points.emplace_back(Point2D(5, 0), 1.5, 3.0);      // Mid-edge, deeper
    path.points.emplace_back(Point2D(10, 0), 0.1, 5.0);     // Corner, shallow cut
    path.points.emplace_back(Point2D(5, 8.66), 0.1, 5.0);   // Top corner, shallow cut
    path.points.emplace_back(Point2D(0, 0), 0.1, 5.0);      // Back to start
    
    path.totalLength = path.calculateLength();
    path.isClosed = true;
    
    EXPECT_TRUE(path.isValid());
    EXPECT_GT(path.totalLength, 0.0);
    EXPECT_EQ(path.getMaxDepth(), 1.5);  // Deepest at mid-edge
    EXPECT_EQ(path.getMinDepth(), 0.1);  // Shallowest at corners
    EXPECT_TRUE(path.isClosed);
}

TEST_F(VCarvePathTest, VCarvePathEdgeCases) {
    // Test with identical consecutive points
    VCarvePath path;
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);
    path.points.emplace_back(Point2D(0, 0), 1.0, 2.0);  // Identical point
    path.points.emplace_back(Point2D(10, 0), 2.0, 1.8);
    
    double length = path.calculateLength();
    EXPECT_NEAR(length, 10.0, 0.001);  // Should skip zero-length segment
}

TEST_F(VCarvePathTest, VCarvePathZeroDepths) {
    // Test path with zero depths (surface-level cuts)
    VCarvePath path;
    path.points.emplace_back(Point2D(0, 0), 0.0, 0.0);
    path.points.emplace_back(Point2D(10, 0), 0.0, 0.0);
    
    EXPECT_TRUE(path.isValid());
    EXPECT_EQ(path.getMaxDepth(), 0.0);
    EXPECT_EQ(path.getMinDepth(), 0.0);
    EXPECT_GT(path.calculateLength(), 0.0);
}
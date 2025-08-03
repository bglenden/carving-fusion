/**
 * test_CoordinateSystemRegression.cpp
 *
 * Critical regression test to prevent coordinate system mismatches
 * that cause medial axis misalignment with shape boundaries.
 *
 * This test ensures that medial axis computation produces results
 * that are spatially consistent with the input polygon boundaries.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <vector>

#include "../../include/geometry/MedialAxisProcessor.h"
#include "../../include/geometry/Point2D.h"

using namespace ChipCarving::Geometry;

class CoordinateSystemRegressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor = std::make_unique<MedialAxisProcessor>();
        processor->setVerbose(false);  // Reduce test output noise
    }

    std::unique_ptr<MedialAxisProcessor> processor;
};

/**
 * CRITICAL REGRESSION TEST: Rectangle Medial Axis Alignment
 * 
 * This test verifies that the medial axis of a rectangle is positioned
 * correctly relative to the rectangle's boundaries. The medial axis
 * should be centered within the rectangle, not offset or rotated.
 * 
 * Historical Bug: When extractProfileVertices() returned local sketch coordinates
 * but MedialAxisProcessor expected world coordinates, the medial axis appeared
 * at the wrong position despite being the correct size and shape.
 */
TEST_F(CoordinateSystemRegressionTest, RectangleMedialAxisAlignment) {
    // Create a rectangle in "world coordinates" (simulating Fusion extraction)
    // Rectangle: 52mm x 22mm, positioned away from origin to detect offset bugs
    double rectWidth = 5.2;   // 52mm in cm
    double rectHeight = 2.2;  // 22mm in cm
    double centerX = 2.0;     // 20mm offset from origin
    double centerY = -3.0;    // -30mm offset from origin
    
    std::vector<Point2D> rectangle = {
        Point2D(centerX - rectWidth/2, centerY - rectHeight/2),  // Bottom-left
        Point2D(centerX + rectWidth/2, centerY - rectHeight/2),  // Bottom-right  
        Point2D(centerX + rectWidth/2, centerY + rectHeight/2),  // Top-right
        Point2D(centerX - rectWidth/2, centerY + rectHeight/2)   // Top-left
    };
    
    // Compute medial axis
    MedialAxisResults results = processor->computeMedialAxis(rectangle);
    
    // Verify computation succeeded
    ASSERT_TRUE(results.success) << "Medial axis computation failed: " << results.errorMessage;
    ASSERT_FALSE(results.chains.empty()) << "No medial axis chains generated";
    
    // Find the centroid of all medial axis points
    double medialCenterX = 0.0, medialCenterY = 0.0;
    int totalPoints = 0;
    
    for (const auto& chain : results.chains) {
        for (const auto& point : chain) {
            medialCenterX += point.x;
            medialCenterY += point.y;
            totalPoints++;
        }
    }
    
    ASSERT_GT(totalPoints, 0) << "No medial axis points found";
    
    medialCenterX /= totalPoints;
    medialCenterY /= totalPoints;
    
    // CRITICAL ASSERTION: Medial axis center should be near rectangle center
    // This catches coordinate system mismatches that cause offset/rotation
    double centerTolerance = 0.5;  // 5mm tolerance in cm
    
    EXPECT_NEAR(medialCenterX, centerX, centerTolerance) 
        << "Medial axis X-center (" << medialCenterX << ") does not match rectangle X-center (" 
        << centerX << "). This indicates a coordinate system mismatch.";
        
    EXPECT_NEAR(medialCenterY, centerY, centerTolerance)
        << "Medial axis Y-center (" << medialCenterY << ") does not match rectangle Y-center (" 
        << centerY << "). This indicates a coordinate system mismatch.";
}

/**
 * CRITICAL REGRESSION TEST: Scale Consistency
 * 
 * Verifies that medial axis results maintain correct scale relationship
 * with input polygon. The medial axis should be proportional to the
 * input shape size, not affected by coordinate transformation bugs.
 */
TEST_F(CoordinateSystemRegressionTest, ScaleConsistency) {
    // Test with two rectangles of different sizes but same aspect ratio
    double aspectRatio = 2.5;  // Width/Height ratio
    (void)aspectRatio; // Suppress unused variable warning
    
    // Small rectangle: 25mm x 10mm
    std::vector<Point2D> smallRect = {
        Point2D(-1.25, -0.5), Point2D(1.25, -0.5),
        Point2D(1.25, 0.5), Point2D(-1.25, 0.5)
    };
    
    // Large rectangle: 50mm x 20mm (2x scale)
    std::vector<Point2D> largeRect = {
        Point2D(-2.5, -1.0), Point2D(2.5, -1.0),
        Point2D(2.5, 1.0), Point2D(-2.5, 1.0)
    };
    
    auto smallResults = processor->computeMedialAxis(smallRect);
    auto largeResults = processor->computeMedialAxis(largeRect);
    
    ASSERT_TRUE(smallResults.success && largeResults.success);
    ASSERT_FALSE(smallResults.chains.empty() && largeResults.chains.empty());
    
    // Calculate bounding boxes of medial axis results
    auto calculateBounds = [](const MedialAxisResults& results) {
        double minX = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();
        
        for (const auto& chain : results.chains) {
            for (const auto& point : chain) {
                minX = std::min(minX, point.x);
                maxX = std::max(maxX, point.x);
                minY = std::min(minY, point.y);
                maxY = std::max(maxY, point.y);
            }
        }
        return std::make_tuple(maxX - minX, maxY - minY);
    };
    
    auto [smallWidth, smallHeight] = calculateBounds(smallResults);
    auto [largeWidth, largeHeight] = calculateBounds(largeResults);
    
    // CRITICAL ASSERTION: Scale should be approximately 2:1
    double scaleTolerance = 0.2;  // 20% tolerance for medial axis variations
    double widthRatio = largeWidth / smallWidth;
    double heightRatio = largeHeight / smallHeight;
    
    EXPECT_NEAR(widthRatio, 2.0, scaleTolerance)
        << "Width scale ratio " << widthRatio << " indicates coordinate system error";
    EXPECT_NEAR(heightRatio, 2.0, scaleTolerance)
        << "Height scale ratio " << heightRatio << " indicates coordinate system error";
}

/**
 * CRITICAL REGRESSION TEST: Coordinate Origin Independence
 * 
 * Verifies that medial axis computation works correctly regardless of
 * the polygon's position relative to the coordinate origin. This catches
 * bugs where local vs world coordinate confusion causes position-dependent errors.
 */
TEST_F(CoordinateSystemRegressionTest, OriginIndependence) {
    // Define a triangle at origin
    std::vector<Point2D> triangleAtOrigin = {
        Point2D(0.0, 1.0),   // Top
        Point2D(-1.0, -1.0), // Bottom-left
        Point2D(1.0, -1.0)   // Bottom-right
    };
    
    // Same triangle translated far from origin
    Point2D offset(10.0, -15.0);  // 100mm, -150mm offset
    std::vector<Point2D> triangleOffset;
    for (const auto& pt : triangleAtOrigin) {
        triangleOffset.push_back(Point2D(pt.x + offset.x, pt.y + offset.y));
    }
    
    auto originResults = processor->computeMedialAxis(triangleAtOrigin);
    auto offsetResults = processor->computeMedialAxis(triangleOffset);
    
    ASSERT_TRUE(originResults.success && offsetResults.success);
    ASSERT_FALSE(originResults.chains.empty() && offsetResults.chains.empty());
    
    // Calculate centroids
    auto calculateCentroid = [](const MedialAxisResults& results) {
        double sumX = 0.0, sumY = 0.0;
        int count = 0;
        for (const auto& chain : results.chains) {
            for (const auto& point : chain) {
                sumX += point.x;
                sumY += point.y;
                count++;
            }
        }
        return Point2D(sumX / count, sumY / count);
    };
    
    Point2D originCentroid = calculateCentroid(originResults);
    Point2D offsetCentroid = calculateCentroid(offsetResults);
    
    // CRITICAL ASSERTION: Offset medial axis should be translated by same offset
    Point2D expectedOffsetCentroid(originCentroid.x + offset.x, originCentroid.y + offset.y);
    
    double positionTolerance = 0.1;  // 1mm tolerance
    EXPECT_NEAR(offsetCentroid.x, expectedOffsetCentroid.x, positionTolerance)
        << "X-coordinate offset error indicates local/world coordinate confusion";
    EXPECT_NEAR(offsetCentroid.y, expectedOffsetCentroid.y, positionTolerance)
        << "Y-coordinate offset error indicates local/world coordinate confusion";
}

/**
 * BOUNDARY TEST: Verify medial axis stays within polygon bounds
 * 
 * Ensures that all medial axis points lie within or very close to
 * the input polygon boundary. This catches coordinate transformation
 * errors that project the medial axis outside the shape.
 */
TEST_F(CoordinateSystemRegressionTest, MedialAxisWithinBounds) {
    // Create a simple square
    double size = 4.0;  // 40mm square
    std::vector<Point2D> square = {
        Point2D(-size/2, -size/2), Point2D(size/2, -size/2),
        Point2D(size/2, size/2), Point2D(-size/2, size/2)
    };
    
    auto results = processor->computeMedialAxis(square);
    ASSERT_TRUE(results.success);
    ASSERT_FALSE(results.chains.empty());
    
    // Calculate polygon bounding box
    double minX = square[0].x, maxX = square[0].x;
    double minY = square[0].y, maxY = square[0].y;
    for (const auto& pt : square) {
        minX = std::min(minX, pt.x);
        maxX = std::max(maxX, pt.x);
        minY = std::min(minY, pt.y);
        maxY = std::max(maxY, pt.y);
    }
    
    // Check all medial axis points are within bounds (with small tolerance)
    double boundsTolerance = 0.01;  // 0.1mm tolerance for numerical precision
    
    for (const auto& chain : results.chains) {
        for (const auto& point : chain) {
            EXPECT_GE(point.x, minX - boundsTolerance)
                << "Medial axis point X=" << point.x << " is outside left bound " << minX;
            EXPECT_LE(point.x, maxX + boundsTolerance)
                << "Medial axis point X=" << point.x << " is outside right bound " << maxX;
            EXPECT_GE(point.y, minY - boundsTolerance)
                << "Medial axis point Y=" << point.y << " is outside bottom bound " << minY;
            EXPECT_LE(point.y, maxY + boundsTolerance)
                << "Medial axis point Y=" << point.y << " is outside top bound " << maxY;
        }
    }
}
/**
 * Comprehensive unit tests for SVGGenerator class
 * Tests coordinate transformations, SVG output generation, and file I/O operations
 * All tests are non-fragile - focus on verifying core functionality without external dependencies
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "../include/geometry/Leaf.h"
#include "../include/geometry/SVGGenerator.h"
#include "../include/geometry/TriArc.h"

using namespace ChipCarving::Geometry;

class SVGGeneratorTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Create test directory for SVG output files
        testDir_ = "test_svg_output";
        std::filesystem::create_directory(testDir_);

        // Create standard test shapes
        leafShape_ = std::make_unique<Leaf>(Point2D(0, 0), Point2D(10, 0), 8.0);

        std::array<double, 3> bulges = {-0.125, -0.125, -0.125};
        triArcShape_ =
            std::make_unique<TriArc>(Point2D(0, 0), Point2D(10, 0), Point2D(5, 8), bulges);
    }

    void TearDown() override {
        // Clean up test files
        try {
            std::filesystem::remove_all(testDir_);
        } catch (...) {
            // Ignore cleanup errors
        }
    }

    std::string testDir_;
    std::unique_ptr<Leaf> leafShape_;
    std::unique_ptr<TriArc> triArcShape_;

    // Helper: Get test file path
    std::string getTestFilePath(const std::string& filename) {
        return testDir_ + "/" + filename;
    }

    // Helper: Check if SVG contains expected elements
    bool containsElement(const std::string& svg, const std::string& element) {
        return svg.find(element) != std::string::npos;
    }

    // Helper: Count occurrences of substring
    size_t countOccurrences(const std::string& str, const std::string& substr) {
        size_t count = 0;
        size_t pos = 0;
        while ((pos = str.find(substr, pos)) != std::string::npos) {
            count++;
            pos += substr.length();
        }
        return count;
    }
};

// ===============================
// Constructor and Initialization Tests
// ===============================

TEST_F(SVGGeneratorTest, DefaultConstructor) {
    SVGGenerator generator;

    std::string svg = generator.generate();

    // Check SVG header and basic structure (accounting for fixed precision formatting)
    EXPECT_TRUE(containsElement(svg, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    EXPECT_TRUE(containsElement(svg, "<svg width=\"400.000\""));
    EXPECT_TRUE(containsElement(svg, "height=\"400.000\""));
    EXPECT_TRUE(containsElement(svg, "xmlns=\"http://www.w3.org/2000/svg\""));
    EXPECT_TRUE(containsElement(svg, "</svg>"));
}

TEST_F(SVGGeneratorTest, CustomDimensionsConstructor) {
    SVGGenerator generator(800, 600, 20.0);

    std::string svg = generator.generate();

    EXPECT_TRUE(containsElement(svg, "<svg width=\"800.000\""));
    EXPECT_TRUE(containsElement(svg, "height=\"600.000\""));
    EXPECT_TRUE(containsElement(svg, "viewBox=\"0 0 800.000 600.000\""));
}

TEST_F(SVGGeneratorTest, GeneratesGridReference) {
    SVGGenerator generator(200, 200);

    std::string svg = generator.generate();

    // Should contain grid lines for reference
    EXPECT_TRUE(containsElement(svg, "<!-- Grid -->"));
    EXPECT_TRUE(containsElement(svg, "stroke=\"#f0f0f0\""));

    // Should have multiple grid lines
    EXPECT_GT(countOccurrences(svg, "stroke=\"#f0f0f0\""), 5);
}

// ===============================
// Coordinate Transformation Tests
// ===============================

TEST_F(SVGGeneratorTest, SetBoundsUpdatesCoordinateSystem) {
    SVGGenerator generator(400, 400);

    // Set bounds for a 20x20 world coordinate system centered at origin
    Point2D min(-10, -10);
    Point2D max(10, 10);
    generator.setBounds(min, max, 2.0);  // 2mm margin

    // Add a point at world origin - should appear near SVG center
    generator.addPoint(Point2D(0, 0), "red", 3.0, "origin");

    std::string svg = generator.generate();

    // Point should be roughly in the center of the 400x400 canvas
    // Exact coordinates depend on scale calculation, but should be around 200,200
    EXPECT_TRUE(containsElement(svg, "cx=\""));
    EXPECT_TRUE(containsElement(svg, "cy=\""));
}

TEST_F(SVGGeneratorTest, SetBoundsHandlesNonSquareRegions) {
    SVGGenerator generator(400, 300);

    // Wide region (aspect ratio 2:1)
    Point2D min(0, 0);
    Point2D max(20, 10);
    generator.setBounds(min, max, 1.0);

    generator.addPoint(Point2D(10, 5), "blue", 2.0);  // Center point

    std::string svg = generator.generate();
    EXPECT_TRUE(containsElement(svg, "fill=\"blue\""));
}

TEST_F(SVGGeneratorTest, SetBoundsWithZeroMargin) {
    SVGGenerator generator;

    Point2D min(5, 5);
    Point2D max(15, 15);
    generator.setBounds(min, max, 0.0);  // No margin

    generator.addPoint(Point2D(5, 5), "green");  // Corner point

    std::string svg = generator.generate();
    EXPECT_TRUE(containsElement(svg, "fill=\"green\""));
}

// ===============================
// Shape Drawing Tests
// ===============================

TEST_F(SVGGeneratorTest, AddLeafGeneratesCorrectPath) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-2, -2), Point2D(12, 10), 1.0);

    generator.addLeaf(*leafShape_, "blue", 2.0);

    std::string svg = generator.generate();

    // Should contain path element with arc commands (using fixed precision format)
    EXPECT_TRUE(containsElement(svg, "<path"));
    EXPECT_TRUE(containsElement(svg, "stroke=\"blue\""));
    EXPECT_TRUE(containsElement(svg, "stroke-width=\"2.000\""));
    EXPECT_TRUE(containsElement(svg, "fill=\"none\""));

    // Leaf should generate arc commands (A in SVG path)
    EXPECT_TRUE(containsElement(svg, " A "));
}

TEST_F(SVGGeneratorTest, AddTriArcGeneratesCorrectPath) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-2, -2), Point2D(12, 10), 1.0);

    generator.addTriArc(*triArcShape_, "red", 1.5);

    std::string svg = generator.generate();

    // Should contain path element (using fixed precision format)
    EXPECT_TRUE(containsElement(svg, "<path"));
    EXPECT_TRUE(containsElement(svg, "stroke=\"red\""));
    EXPECT_TRUE(containsElement(svg, "stroke-width=\"1.500\""));

    // TriArc should generate multiple arc commands
    EXPECT_GE(countOccurrences(svg, " A "), 1);
}

TEST_F(SVGGeneratorTest, AddMultipleShapesInOneGenerator) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-5, -5), Point2D(15, 15), 2.0);

    generator.addLeaf(*leafShape_, "blue", 1.0);
    generator.addTriArc(*triArcShape_, "red", 1.0);

    std::string svg = generator.generate();

    // Should contain both shapes
    EXPECT_TRUE(containsElement(svg, "stroke=\"blue\""));
    EXPECT_TRUE(containsElement(svg, "stroke=\"red\""));
    EXPECT_GE(countOccurrences(svg, "<path"), 2);
}

// ===============================
// Debug Marker Tests
// ===============================

TEST_F(SVGGeneratorTest, AddDebugMarkersForLeaf) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-2, -2), Point2D(12, 10), 1.0);

    generator.addLeaf(*leafShape_);
    generator.addDebugMarkers(*leafShape_);

    std::string svg = generator.generate();

    // Should contain debug markers (circles for vertices, center)
    EXPECT_TRUE(containsElement(svg, "<circle"));
    EXPECT_TRUE(containsElement(svg, "fill=\"red\""));  // Default vertex color

    // Should have markers for both foci
    EXPECT_GE(countOccurrences(svg, "<circle"), 2);
}

TEST_F(SVGGeneratorTest, AddTriArcDebugMarkers) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-2, -2), Point2D(12, 10), 1.0);

    generator.addTriArc(*triArcShape_);
    generator.addTriArcDebugMarkers(*triArcShape_);

    std::string svg = generator.generate();

    // Should contain vertex markers and arc centers
    EXPECT_TRUE(containsElement(svg, "<circle"));
    EXPECT_GE(countOccurrences(svg, "<circle"), 3);  // At least 3 vertices
}

// ===============================
// Primitive Shape Tests
// ===============================

TEST_F(SVGGeneratorTest, AddPointWithLabel) {
    SVGGenerator generator;

    generator.addPoint(Point2D(10, 20), "green", 4.0, "Test Point");

    std::string svg = generator.generate();

    // Should contain circle and text elements (using fixed precision format)
    EXPECT_TRUE(containsElement(svg, "<circle"));
    EXPECT_TRUE(containsElement(svg, "fill=\"green\""));
    EXPECT_TRUE(containsElement(svg, "r=\"4.000\""));
    EXPECT_TRUE(containsElement(svg, "<text"));
    EXPECT_TRUE(containsElement(svg, "Test Point"));
}

TEST_F(SVGGeneratorTest, AddLine) {
    SVGGenerator generator;

    generator.addLine(Point2D(0, 0), Point2D(10, 10), "purple", 2.5, "dashed");

    std::string svg = generator.generate();

    EXPECT_TRUE(containsElement(svg, "<line"));
    EXPECT_TRUE(containsElement(svg, "stroke=\"purple\""));
    EXPECT_TRUE(containsElement(svg, "stroke-width=\"2.500\""));
    EXPECT_TRUE(containsElement(svg, "dashed"));
}

TEST_F(SVGGeneratorTest, AddArc) {
    SVGGenerator generator;

    // Add a 90-degree arc
    generator.addArc(Point2D(5, 5), 3.0, 0, M_PI / 2, false, "orange", 1.5);

    std::string svg = generator.generate();

    EXPECT_TRUE(containsElement(svg, "<path"));
    EXPECT_TRUE(containsElement(svg, "stroke=\"orange\""));
    EXPECT_TRUE(containsElement(svg, "stroke-width=\"1.500\""));
    EXPECT_TRUE(containsElement(svg, " A "));  // Arc command
}

TEST_F(SVGGeneratorTest, AddText) {
    SVGGenerator generator;

    generator.addText(Point2D(15, 25), "Sample Text", "black", 14.0);

    std::string svg = generator.generate();

    EXPECT_TRUE(containsElement(svg, "<text"));
    EXPECT_TRUE(containsElement(svg, "fill=\"black\""));
    EXPECT_TRUE(containsElement(svg, "font-size=\"14.000\""));
    EXPECT_TRUE(containsElement(svg, "Sample Text"));
}

// ===============================
// File I/O Tests
// ===============================

TEST_F(SVGGeneratorTest, SaveToFileCreatesValidFile) {
    SVGGenerator generator;
    generator.addPoint(Point2D(0, 0), "red", 5.0, "test");

    std::string filename = getTestFilePath("test_output.svg");

    bool success = generator.saveToFile(filename);

    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(filename));

    // Verify file content
    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    EXPECT_TRUE(containsElement(content, "<?xml version=\"1.0\""));
    EXPECT_TRUE(containsElement(content, "fill=\"red\""));
    EXPECT_TRUE(containsElement(content, "test"));
}

TEST_F(SVGGeneratorTest, SaveToFileHandlesInvalidPath) {
    SVGGenerator generator;
    generator.addPoint(Point2D(0, 0));

    // Try to save to invalid path (non-existent directory)
    std::string invalidPath = "/nonexistent/directory/test.svg";

    bool success = generator.saveToFile(invalidPath);

    EXPECT_FALSE(success);
}

TEST_F(SVGGeneratorTest, SaveToFileOverwritesExistingFile) {
    SVGGenerator generator1;
    generator1.addPoint(Point2D(0, 0), "red");

    SVGGenerator generator2;
    generator2.addPoint(Point2D(5, 5), "blue");

    std::string filename = getTestFilePath("overwrite_test.svg");

    // Save first file
    EXPECT_TRUE(generator1.saveToFile(filename));

    // Overwrite with second file
    EXPECT_TRUE(generator2.saveToFile(filename));

    // Verify content is from second generator
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    EXPECT_TRUE(containsElement(content, "fill=\"blue\""));
    EXPECT_FALSE(containsElement(content, "fill=\"red\""));
}

// ===============================
// SVGComparator Tests
// ===============================

TEST_F(SVGGeneratorTest, SVGComparatorExtractsNumbers) {
    std::string svgContent = R"(
        <path d="M 10.5 20.25 L 30.125 40.0 A 5.5 5.5 0 0 1 45.75 55.0 Z"/>
        <circle cx="15.333" cy="25.667" r="3.14159"/>
    )";

    std::vector<double> numbers = SVGComparator::extractNumbers(svgContent);

    // Should extract all numerical values
    EXPECT_GT(numbers.size(), 8);

    // Check some specific values
    EXPECT_TRUE(std::find_if(numbers.begin(), numbers.end(),
                             [](double d) { return std::abs(d - 10.5) < 1e-9; }) != numbers.end());
    EXPECT_TRUE(std::find_if(numbers.begin(), numbers.end(), [](double d) {
                    return std::abs(d - 3.14159) < 1e-5;
                }) != numbers.end());
}

TEST_F(SVGGeneratorTest, SVGComparatorCompareNumbers) {
    std::vector<double> numbers1 = {1.0, 2.5, 3.14159, 4.0};
    std::vector<double> numbers2 = {1.000001, 2.500001, 3.141591, 4.000001};
    std::vector<double> numbers3 = {1.0, 2.5, 3.0, 4.0};  // Different value

    // Should be equal within tolerance
    EXPECT_TRUE(SVGComparator::compareNumbers(numbers1, numbers2, 1e-5));

    // Should not be equal - significant difference
    EXPECT_FALSE(SVGComparator::compareNumbers(numbers1, numbers3, 1e-5));

    // Different sizes
    std::vector<double> numbers4 = {1.0, 2.5, 3.14159};
    EXPECT_FALSE(SVGComparator::compareNumbers(numbers1, numbers4, 1e-5));
}

TEST_F(SVGGeneratorTest, SVGComparatorCompareIdenticalFiles) {
    SVGGenerator generator;
    generator.addLeaf(*leafShape_, "black", 1.0);

    std::string file1 = getTestFilePath("identical1.svg");
    std::string file2 = getTestFilePath("identical2.svg");

    // Save same content to both files
    EXPECT_TRUE(generator.saveToFile(file1));
    EXPECT_TRUE(generator.saveToFile(file2));

    // Should be identical
    EXPECT_TRUE(SVGComparator::compare(file1, file2));
}

TEST_F(SVGGeneratorTest, SVGComparatorCompareDifferentFiles) {
    SVGGenerator generator1;
    generator1.addLeaf(*leafShape_, "black", 1.0);

    SVGGenerator generator2;
    generator2.addTriArc(*triArcShape_, "black", 1.0);

    std::string file1 = getTestFilePath("different1.svg");
    std::string file2 = getTestFilePath("different2.svg");

    EXPECT_TRUE(generator1.saveToFile(file1));
    EXPECT_TRUE(generator2.saveToFile(file2));

    // Should be different
    EXPECT_FALSE(SVGComparator::compare(file1, file2));
}

// ===============================
// Edge Cases and Integration Tests
// ===============================

TEST_F(SVGGeneratorTest, HandlesEmptyGeneration) {
    SVGGenerator generator;

    std::string svg = generator.generate();

    // Should still be valid SVG even with no content
    EXPECT_TRUE(containsElement(svg, "<?xml version"));
    EXPECT_TRUE(containsElement(svg, "<svg"));
    EXPECT_TRUE(containsElement(svg, "</svg>"));
}

TEST_F(SVGGeneratorTest, HandlesLargeCoordinates) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-1000, -1000), Point2D(1000, 1000), 100);

    generator.addPoint(Point2D(500, -500), "red", 10.0);

    std::string svg = generator.generate();
    EXPECT_TRUE(containsElement(svg, "fill=\"red\""));
}

TEST_F(SVGGeneratorTest, HandlesVerySmallCoordinates) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-0.001, -0.001), Point2D(0.001, 0.001), 0.0001);

    generator.addPoint(Point2D(0.0005, -0.0005), "blue", 1.0);

    std::string svg = generator.generate();
    EXPECT_TRUE(containsElement(svg, "fill=\"blue\""));
}

TEST_F(SVGGeneratorTest, GeneratesComplexShapeComposition) {
    SVGGenerator generator;
    generator.setBounds(Point2D(-5, -5), Point2D(15, 15), 2.0);

    // Add multiple shapes and debug markers
    generator.addLeaf(*leafShape_, "blue", 1.5);
    generator.addTriArc(*triArcShape_, "red", 1.0);
    generator.addDebugMarkers(*leafShape_);
    generator.addTriArcDebugMarkers(*triArcShape_);

    // Add additional primitives
    generator.addLine(Point2D(0, 0), Point2D(10, 8), "gray", 0.5, "dashed");
    generator.addText(Point2D(5, -3), "Composition Test", "black", 10);

    std::string svg = generator.generate();

    // Verify all elements are present
    EXPECT_TRUE(containsElement(svg, "stroke=\"blue\""));
    EXPECT_TRUE(containsElement(svg, "stroke=\"red\""));
    EXPECT_TRUE(containsElement(svg, "stroke=\"gray\""));
    EXPECT_TRUE(containsElement(svg, "dashed"));
    EXPECT_TRUE(containsElement(svg, "Composition Test"));

    // Should have multiple paths and circles
    EXPECT_GE(countOccurrences(svg, "<path"), 2);
    EXPECT_GE(countOccurrences(svg, "<circle"), 3);
}

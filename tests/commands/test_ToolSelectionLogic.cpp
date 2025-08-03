/**
 * Tests for tool selection and configuration logic
 * Pure business logic without UI dependencies
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cmath>

namespace ChipCarving {
namespace Commands {

// Business logic for tool configuration (extracted from UI code)
struct ToolConfiguration {
    std::string name;
    double angle;
    double defaultDepth;
    
    static ToolConfiguration fromToolName(const std::string& toolName) {
        ToolConfiguration config;
        config.name = toolName;
        
        if (toolName == "90° V-bit") {
            config.angle = 90.0;
            config.defaultDepth = 3.0;  // mm
        } else if (toolName == "60° V-bit") {
            config.angle = 60.0;
            config.defaultDepth = 5.0;  // mm - deeper for sharper angle
        } else if (toolName == "45° V-bit") {
            config.angle = 45.0;
            config.defaultDepth = 6.0;  // mm - even deeper
        } else if (toolName == "30° V-bit") {
            config.angle = 30.0;
            config.defaultDepth = 8.0;  // mm - deepest for sharpest angle
        } else {
            // Default/unknown tool
            config.angle = 90.0;
            config.defaultDepth = 3.0;
        }
        
        return config;
    }
    
    // Calculate cut width at a given depth
    double cutWidthAtDepth(double depth) const {
        // Width = 2 * depth * tan(angle/2)
        double angleRad = angle * M_PI / 180.0;
        return 2.0 * depth * std::tan(angleRad / 2.0);
    }
    
    // Calculate required depth for a given cut width
    double depthForWidth(double width) const {
        // depth = width / (2 * tan(angle/2))
        double angleRad = angle * M_PI / 180.0;
        return width / (2.0 * std::tan(angleRad / 2.0));
    }
};

} // namespace Commands
} // namespace ChipCarving

using namespace ChipCarving::Commands;

class ToolSelectionTest : public ::testing::Test {
protected:
    std::vector<std::string> standardTools = {
        "90° V-bit",
        "60° V-bit", 
        "45° V-bit",
        "30° V-bit"
    };
};

TEST_F(ToolSelectionTest, ToolNameToAngleMapping) {
    // Test standard tool mappings
    EXPECT_DOUBLE_EQ(90.0, ToolConfiguration::fromToolName("90° V-bit").angle);
    EXPECT_DOUBLE_EQ(60.0, ToolConfiguration::fromToolName("60° V-bit").angle);
    EXPECT_DOUBLE_EQ(45.0, ToolConfiguration::fromToolName("45° V-bit").angle);
    EXPECT_DOUBLE_EQ(30.0, ToolConfiguration::fromToolName("30° V-bit").angle);
    
    // Test unknown tool defaults to 90°
    EXPECT_DOUBLE_EQ(90.0, ToolConfiguration::fromToolName("Unknown Tool").angle);
    EXPECT_DOUBLE_EQ(90.0, ToolConfiguration::fromToolName("").angle);
}

TEST_F(ToolSelectionTest, DefaultDepthsByAngle) {
    // Sharper tools should have deeper default depths
    auto tool90 = ToolConfiguration::fromToolName("90° V-bit");
    auto tool60 = ToolConfiguration::fromToolName("60° V-bit");
    auto tool45 = ToolConfiguration::fromToolName("45° V-bit");
    auto tool30 = ToolConfiguration::fromToolName("30° V-bit");
    
    EXPECT_LT(tool90.defaultDepth, tool60.defaultDepth);
    EXPECT_LT(tool60.defaultDepth, tool45.defaultDepth);
    EXPECT_LT(tool45.defaultDepth, tool30.defaultDepth);
}

TEST_F(ToolSelectionTest, CutWidthCalculations) {
    auto tool90 = ToolConfiguration::fromToolName("90° V-bit");
    auto tool60 = ToolConfiguration::fromToolName("60° V-bit");
    
    // For 90° V-bit at 1mm depth, width should be 2mm
    EXPECT_NEAR(2.0, tool90.cutWidthAtDepth(1.0), 0.001);
    
    // For 90° V-bit at 3mm depth, width should be 6mm
    EXPECT_NEAR(6.0, tool90.cutWidthAtDepth(3.0), 0.001);
    
    // For 60° V-bit, width is narrower at same depth
    double width60at1mm = tool60.cutWidthAtDepth(1.0);
    EXPECT_LT(width60at1mm, 2.0);
    EXPECT_NEAR(1.1547, width60at1mm, 0.001); // 2 * tan(30°) ≈ 1.1547
}

TEST_F(ToolSelectionTest, DepthForWidthCalculations) {
    auto tool90 = ToolConfiguration::fromToolName("90° V-bit");
    auto tool60 = ToolConfiguration::fromToolName("60° V-bit");
    
    // For 90° V-bit, 2mm width requires 1mm depth
    EXPECT_NEAR(1.0, tool90.depthForWidth(2.0), 0.001);
    
    // For 60° V-bit, same width requires more depth
    double depth60for2mm = tool60.depthForWidth(2.0);
    EXPECT_GT(depth60for2mm, 1.0);
    EXPECT_NEAR(1.732, depth60for2mm, 0.001); // 1 / tan(30°) ≈ 1.732
}

TEST_F(ToolSelectionTest, ReciprocalRelationship) {
    // Test that cutWidthAtDepth and depthForWidth are inverses
    for (const auto& toolName : standardTools) {
        auto tool = ToolConfiguration::fromToolName(toolName);
        
        // Test various depths
        for (double depth = 0.5; depth <= 5.0; depth += 0.5) {
            double width = tool.cutWidthAtDepth(depth);
            double calculatedDepth = tool.depthForWidth(width);
            EXPECT_NEAR(depth, calculatedDepth, 0.0001) 
                << "Failed for " << toolName << " at depth " << depth;
        }
    }
}

TEST_F(ToolSelectionTest, EdgeCases) {
    auto tool90 = ToolConfiguration::fromToolName("90° V-bit");
    
    // Zero depth should give zero width
    EXPECT_DOUBLE_EQ(0.0, tool90.cutWidthAtDepth(0.0));
    
    // Zero width should give zero depth
    EXPECT_DOUBLE_EQ(0.0, tool90.depthForWidth(0.0));
    
    // Negative values should work mathematically (even if not physically meaningful)
    EXPECT_LT(tool90.cutWidthAtDepth(-1.0), 0.0);
    EXPECT_LT(tool90.depthForWidth(-2.0), 0.0);
}
/**
 * Mock implementations tests
 * Tests for mock adapters used in unit testing
 */

#include <gtest/gtest.h>

#include "MockAdapters.h"

// Basic mock tests
TEST(MockAdaptersTest, MockLoggerCapturesMessages) {
    MockLogger logger;

    logger.logInfo("Test info message");
    logger.logError("Test error message");

    EXPECT_EQ(logger.infoMessages.size(), 1);
    EXPECT_EQ(logger.errorMessages.size(), 1);
    EXPECT_EQ(logger.infoMessages[0], "Test info message");
    EXPECT_EQ(logger.errorMessages[0], "Test error message");
}

TEST(MockAdaptersTest, MockUITracksMessageBoxCalls) {
    MockUserInterface ui;

    ui.showMessageBox("Test Title", "Test Message");

    EXPECT_EQ(ui.messageBoxCallCount, 1);
    EXPECT_EQ(ui.lastMessageBoxTitle, "Test Title");
    EXPECT_EQ(ui.lastMessageBoxMessage, "Test Message");
}

TEST(MockAdaptersTest, MockWorkspaceTracksSketchOperations) {
    MockWorkspace workspace;

    auto sketch = workspace.createSketch("Test Sketch");
    ASSERT_NE(sketch, nullptr);

    // Cast to MockSketch to access test helpers
    auto mockSketch = dynamic_cast<MockSketch*>(sketch.get());
    ASSERT_NE(mockSketch, nullptr);

    mockSketch->addLineToSketch(0, 0, 10, 10);
    mockSketch->finishSketch();

    EXPECT_EQ(workspace.createSketchCallCount, 1);
    EXPECT_EQ(workspace.lastSketchName, "Test Sketch");
    EXPECT_EQ(mockSketch->lines.size(), 1);
    EXPECT_EQ(mockSketch->finishSketchCallCount, 1);
}

TEST(MockAdaptersTest, MockFactoryCreatesValidObjects) {
    MockFactory factory;

    auto logger = factory.createLogger();
    auto ui = factory.createUserInterface();
    auto workspace = factory.createWorkspace();

    EXPECT_NE(logger, nullptr);
    EXPECT_NE(ui, nullptr);
    EXPECT_NE(workspace, nullptr);
}

TEST(MockAdaptersTest, MockSketchGetSketchCurveEntityIds) {
    MockSketch sketch("Test Sketch");
    
    // Initially no curve entity IDs
    auto entityIds = sketch.getSketchCurveEntityIds();
    EXPECT_TRUE(entityIds.empty());
    
    // Set some mock curve entity IDs
    sketch.mockCurveEntityIds = {"curve1", "curve2", "curve3"};
    entityIds = sketch.getSketchCurveEntityIds();
    
    EXPECT_EQ(entityIds.size(), 3);
    EXPECT_EQ(entityIds[0], "curve1");
    EXPECT_EQ(entityIds[1], "curve2");
    EXPECT_EQ(entityIds[2], "curve3");
    
    // Test reset clears the mock data
    sketch.reset();
    entityIds = sketch.getSketchCurveEntityIds();
    EXPECT_TRUE(entityIds.empty());
}
/**
 * Unit tests for Enhanced UI sketch selection validation logic
 * Tests pure business logic without GUI dependencies or complex mocks
 */

#include <gtest/gtest.h>

#include "adapters/IFusionInterface.h"

namespace ChipCarving {
namespace Commands {
namespace Test {

class SketchSelectionValidationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Initialize valid selection
        validSelection.isValid = true;
        validSelection.closedPathCount = 3;
        validSelection.selectedEntityIds = {"profile_1", "profile_2", "profile_3"};
        validSelection.errorMessage = "";

        // Initialize invalid selection
        invalidSelection.isValid = false;
        invalidSelection.closedPathCount = 0;
        invalidSelection.selectedEntityIds.clear();
        invalidSelection.errorMessage = "No closed profiles selected";

        // Initialize edge case selection
        edgeCaseSelection.isValid = true;
        edgeCaseSelection.closedPathCount = 1;
        edgeCaseSelection.selectedEntityIds = {"profile_1"};
        edgeCaseSelection.errorMessage = "";
    }

    Adapters::SketchSelection validSelection;
    Adapters::SketchSelection invalidSelection;
    Adapters::SketchSelection edgeCaseSelection;
};

// Test valid selection properties
TEST_F(SketchSelectionValidationTest, ValidSelectionProperties) {
    EXPECT_TRUE(validSelection.isValid);
    EXPECT_EQ(validSelection.closedPathCount, 3);
    EXPECT_EQ(validSelection.selectedEntityIds.size(), 3);
    EXPECT_TRUE(validSelection.errorMessage.empty());

    // Verify entity IDs are properly formatted
    for (const auto& id : validSelection.selectedEntityIds) {
        EXPECT_FALSE(id.empty());
        EXPECT_TRUE(id.find("profile_") == 0);  // Should start with "profile_"
    }
}

// Test invalid selection properties
TEST_F(SketchSelectionValidationTest, InvalidSelectionProperties) {
    EXPECT_FALSE(invalidSelection.isValid);
    EXPECT_EQ(invalidSelection.closedPathCount, 0);
    EXPECT_TRUE(invalidSelection.selectedEntityIds.empty());
    EXPECT_FALSE(invalidSelection.errorMessage.empty());
}

// Test selection count validation logic
TEST_F(SketchSelectionValidationTest, SelectionCountValidation) {
    Adapters::SketchSelection selection;

    // Test minimum valid count (at least 1)
    selection.closedPathCount = 1;
    selection.selectedEntityIds = {"profile_1"};
    EXPECT_GE(selection.closedPathCount, 1);
    EXPECT_EQ(selection.selectedEntityIds.size(), selection.closedPathCount);

    // Test multiple selections
    selection.closedPathCount = 5;
    selection.selectedEntityIds = {"profile_1", "profile_2", "profile_3", "profile_4", "profile_5"};
    EXPECT_EQ(selection.selectedEntityIds.size(), selection.closedPathCount);

    // Test large selection count
    selection.closedPathCount = 100;
    selection.selectedEntityIds.clear();
    for (int i = 1; i <= 100; i++) {
        selection.selectedEntityIds.push_back("profile_" + std::to_string(i));
    }
    EXPECT_EQ(selection.selectedEntityIds.size(), selection.closedPathCount);
    EXPECT_EQ(selection.selectedEntityIds.size(), 100);
}

// Test error message validation
TEST_F(SketchSelectionValidationTest, ErrorMessageValidation) {
    Adapters::SketchSelection selection;

    // Test common error messages
    std::vector<std::string> validErrorMessages = {
        "No closed profiles selected",
        "Selected entity is not a valid closed profile. Click INSIDE blue shaded regions only.",
        "Selected profile has no area (not closed)",
        "FILTER ERROR: Selected entity type SketchCurve is not a Profile. The \"Profiles\" filter "
        "should prevent this.",
        "Profile selection input not found",
        "Error processing profile selection"};

    for (const auto& errorMsg : validErrorMessages) {
        selection.isValid = false;
        selection.errorMessage = errorMsg;
        selection.closedPathCount = 0;
        selection.selectedEntityIds.clear();

        EXPECT_FALSE(selection.isValid);
        EXPECT_EQ(selection.errorMessage, errorMsg);
        EXPECT_FALSE(selection.errorMessage.empty());
    }
}

// Test consistency between isValid flag and other fields
TEST_F(SketchSelectionValidationTest, ConsistencyValidation) {
    Adapters::SketchSelection selection;

    // Valid selection should have consistent fields
    selection.isValid = true;
    selection.closedPathCount = 2;
    selection.selectedEntityIds = {"profile_1", "profile_2"};
    selection.errorMessage = "";

    EXPECT_TRUE(selection.isValid);
    EXPECT_GT(selection.closedPathCount, 0);
    EXPECT_FALSE(selection.selectedEntityIds.empty());
    EXPECT_TRUE(selection.errorMessage.empty());
    EXPECT_EQ(selection.selectedEntityIds.size(), selection.closedPathCount);

    // Invalid selection should have consistent fields
    selection.isValid = false;
    selection.closedPathCount = 0;
    selection.selectedEntityIds.clear();
    selection.errorMessage = "Some error occurred";

    EXPECT_FALSE(selection.isValid);
    EXPECT_EQ(selection.closedPathCount, 0);
    EXPECT_TRUE(selection.selectedEntityIds.empty());
    EXPECT_FALSE(selection.errorMessage.empty());
}

// Test entity ID format validation
TEST_F(SketchSelectionValidationTest, EntityIdFormatValidation) {
    Adapters::SketchSelection selection;
    selection.isValid = true;

    // Test valid ID formats
    std::vector<std::string> validIds = {"profile_1", "profile_2", "profile_10", "profile_999",
                                         "profile_1234567890"};

    selection.selectedEntityIds = validIds;
    selection.closedPathCount = validIds.size();

    for (const auto& id : selection.selectedEntityIds) {
        EXPECT_FALSE(id.empty());
        EXPECT_TRUE(id.find("profile_") == 0);

        // Extract numeric part
        std::string numericPart = id.substr(8);  // Skip "profile_"
        EXPECT_FALSE(numericPart.empty());

        // Verify numeric part is valid
        bool isNumeric = true;
        for (char c : numericPart) {
            if (!std::isdigit(c)) {
                isNumeric = false;
                break;
            }
        }
        EXPECT_TRUE(isNumeric);
    }
}

// Test edge cases
TEST_F(SketchSelectionValidationTest, EdgeCases) {
    Adapters::SketchSelection selection;

    // Test single selection (minimum valid case)
    selection.isValid = true;
    selection.closedPathCount = 1;
    selection.selectedEntityIds = {"profile_1"};
    selection.errorMessage = "";

    EXPECT_TRUE(selection.isValid);
    EXPECT_EQ(selection.closedPathCount, 1);
    EXPECT_EQ(selection.selectedEntityIds.size(), 1);
    EXPECT_TRUE(selection.errorMessage.empty());

    // Test empty selection with error
    selection.isValid = false;
    selection.closedPathCount = 0;
    selection.selectedEntityIds.clear();
    selection.errorMessage = "No profiles found";

    EXPECT_FALSE(selection.isValid);
    EXPECT_EQ(selection.closedPathCount, 0);
    EXPECT_TRUE(selection.selectedEntityIds.empty());
    EXPECT_FALSE(selection.errorMessage.empty());
}

// Test selection copying and assignment
TEST_F(SketchSelectionValidationTest, SelectionCopyAndAssignment) {
    Adapters::SketchSelection selection1 = validSelection;
    Adapters::SketchSelection selection2;

    // Modify selection1
    selection1.isValid = false;
    selection1.closedPathCount = 1;
    selection1.selectedEntityIds = {"profile_modified"};
    selection1.errorMessage = "Modified error";

    // Copy assignment
    selection2 = selection1;

    // Verify all fields copied correctly
    EXPECT_EQ(selection2.isValid, selection1.isValid);
    EXPECT_EQ(selection2.closedPathCount, selection1.closedPathCount);
    EXPECT_EQ(selection2.selectedEntityIds, selection1.selectedEntityIds);
    EXPECT_EQ(selection2.errorMessage, selection1.errorMessage);

    // Verify independence
    selection2.closedPathCount = 999;
    EXPECT_NE(selection1.closedPathCount, selection2.closedPathCount);
}

// Test selection state transitions
TEST_F(SketchSelectionValidationTest, SelectionStateTransitions) {
    Adapters::SketchSelection selection;

    // Start with invalid state
    selection.isValid = false;
    selection.closedPathCount = 0;
    selection.selectedEntityIds.clear();
    selection.errorMessage = "Initial error";

    EXPECT_FALSE(selection.isValid);
    EXPECT_EQ(selection.closedPathCount, 0);

    // Transition to valid state
    selection.isValid = true;
    selection.closedPathCount = 2;
    selection.selectedEntityIds = {"profile_1", "profile_2"};
    selection.errorMessage = "";

    EXPECT_TRUE(selection.isValid);
    EXPECT_EQ(selection.closedPathCount, 2);
    EXPECT_TRUE(selection.errorMessage.empty());

    // Transition back to invalid state
    selection.isValid = false;
    selection.closedPathCount = 0;
    selection.selectedEntityIds.clear();
    selection.errorMessage = "New error";

    EXPECT_FALSE(selection.isValid);
    EXPECT_EQ(selection.closedPathCount, 0);
    EXPECT_FALSE(selection.errorMessage.empty());
}

// Test large selection handling
TEST_F(SketchSelectionValidationTest, LargeSelectionHandling) {
    Adapters::SketchSelection selection;
    selection.isValid = true;
    selection.errorMessage = "";

    // Test with large number of selections
    const int LARGE_COUNT = 1000;
    selection.closedPathCount = LARGE_COUNT;

    for (int i = 1; i <= LARGE_COUNT; i++) {
        selection.selectedEntityIds.push_back("profile_" + std::to_string(i));
    }

    EXPECT_EQ(selection.selectedEntityIds.size(), LARGE_COUNT);
    EXPECT_EQ(selection.closedPathCount, LARGE_COUNT);

    // Verify all IDs are properly formatted
    for (int i = 0; i < LARGE_COUNT; i++) {
        const std::string& id = selection.selectedEntityIds[i];
        EXPECT_TRUE(id.find("profile_") == 0);

        std::string expectedId = "profile_" + std::to_string(i + 1);
        EXPECT_EQ(id, expectedId);
    }
}

}  // namespace Test
}  // namespace Commands
}  // namespace ChipCarving
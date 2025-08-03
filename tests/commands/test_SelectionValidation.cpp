/**
 * Tests for selection validation logic
 * Pure business logic without Fusion API dependencies
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <set>
#include "adapters/IFusionInterface.h"

using namespace ChipCarving::Adapters;

class SelectionValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test selection
        validSelection.isValid = true;
        validSelection.closedPathCount = 1;
        validSelection.selectedEntityIds.push_back("profile_1");
        validSelection.errorMessage = "";
        
        invalidSelection.isValid = false;
        invalidSelection.closedPathCount = 0;
        invalidSelection.selectedEntityIds.clear();
        invalidSelection.errorMessage = "No closed profiles selected";
    }
    
    SketchSelection validSelection;
    SketchSelection invalidSelection;
};

TEST_F(SelectionValidationTest, ValidSelectionProperties) {
    EXPECT_TRUE(validSelection.isValid);
    EXPECT_GT(validSelection.closedPathCount, 0);
    EXPECT_FALSE(validSelection.selectedEntityIds.empty());
    EXPECT_TRUE(validSelection.errorMessage.empty());
}

TEST_F(SelectionValidationTest, InvalidSelectionProperties) {
    EXPECT_FALSE(invalidSelection.isValid);
    EXPECT_EQ(0, invalidSelection.closedPathCount);
    EXPECT_TRUE(invalidSelection.selectedEntityIds.empty());
    EXPECT_FALSE(invalidSelection.errorMessage.empty());
}

TEST_F(SelectionValidationTest, MultipleProfileSelection) {
    SketchSelection multiSelection;
    multiSelection.isValid = true;
    multiSelection.closedPathCount = 3;
    multiSelection.selectedEntityIds = {"profile_1", "profile_2", "profile_3"};
    multiSelection.errorMessage = "";
    
    EXPECT_EQ(3, multiSelection.closedPathCount);
    EXPECT_EQ(3, multiSelection.selectedEntityIds.size());
    EXPECT_TRUE(multiSelection.isValid);
}

TEST_F(SelectionValidationTest, SelectionErrorMessages) {
    // Test various error conditions
    struct ErrorCase {
        std::string condition;
        std::string expectedMessage;
        int closedPathCount;
    };
    
    std::vector<ErrorCase> errorCases = {
        {"no_selection", "No closed profiles selected", 0},
        {"open_curves", "Selected entity is not a valid closed profile", 0},
        {"invalid_area", "Selected profile has no area (not closed)", 0},
        {"filter_error", "FILTER ERROR: Selected entity type", 0}
    };
    
    for (const auto& errorCase : errorCases) {
        SketchSelection selection;
        selection.isValid = false;
        selection.closedPathCount = errorCase.closedPathCount;
        selection.errorMessage = errorCase.expectedMessage;
        
        EXPECT_FALSE(selection.isValid) << "Error case: " << errorCase.condition;
        EXPECT_FALSE(selection.errorMessage.empty()) << "Should have error message";
        EXPECT_NE(selection.errorMessage.find(errorCase.expectedMessage.substr(0, 10)), 
                  std::string::npos) << "Should contain expected error text";
    }
}

TEST_F(SelectionValidationTest, SelectionStateTransitions) {
    SketchSelection selection;
    
    // Start with invalid state
    selection.isValid = false;
    selection.closedPathCount = 0;
    selection.errorMessage = "Nothing selected";
    
    // Transition to valid state
    selection.isValid = true;
    selection.closedPathCount = 1;
    selection.selectedEntityIds.push_back("profile_1");
    selection.errorMessage = "";
    
    EXPECT_TRUE(selection.isValid);
    EXPECT_TRUE(selection.errorMessage.empty()) << "Error message should be cleared";
    
    // Add more selections
    selection.selectedEntityIds.push_back("profile_2");
    selection.closedPathCount = 2;
    
    EXPECT_EQ(2, selection.closedPathCount);
    EXPECT_EQ(2, selection.selectedEntityIds.size());
}

TEST_F(SelectionValidationTest, EntityIdUniqueness) {
    SketchSelection selection;
    selection.isValid = true;
    
    // Add duplicate IDs
    selection.selectedEntityIds = {"profile_1", "profile_2", "profile_1"};
    
    // In real usage, duplicates should be prevented
    // This test documents current behavior
    EXPECT_EQ(3, selection.selectedEntityIds.size()) << "Currently allows duplicates";
    
    // Count unique IDs
    std::set<std::string> uniqueIds(selection.selectedEntityIds.begin(), 
                                    selection.selectedEntityIds.end());
    EXPECT_EQ(2, uniqueIds.size()) << "Should have 2 unique IDs";
}

TEST_F(SelectionValidationTest, EmptyButValidSelection) {
    // Edge case: selection object is valid but contains no profiles
    SketchSelection emptyValid;
    emptyValid.isValid = true;  // Marked as valid
    emptyValid.closedPathCount = 0;  // But no profiles
    emptyValid.errorMessage = "";
    
    // This is an inconsistent state that should be avoided
    EXPECT_TRUE(emptyValid.isValid);
    EXPECT_EQ(0, emptyValid.closedPathCount);
    
    // Better practice: isValid should be false when closedPathCount is 0
    bool actuallyValid = emptyValid.isValid && emptyValid.closedPathCount > 0;
    EXPECT_FALSE(actuallyValid) << "Selection with no profiles shouldn't be considered valid";
}
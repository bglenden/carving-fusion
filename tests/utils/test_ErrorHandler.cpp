/**
 * test_ErrorHandler.cpp
 *
 * Non-fragile unit tests for ErrorHandler utility class
 * Tests exception handling patterns and error reporting
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <sstream>
#include "../../include/utils/ErrorHandler.h"

namespace ChipCarving {
namespace Utils {
namespace test {

class ErrorHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        errorMessages_.clear();
        callbackInvoked_ = false;
    }

    std::vector<std::string> errorMessages_;
    bool callbackInvoked_;
    
    // Helper function to capture error messages
    void captureError(const std::string& msg) {
        errorMessages_.push_back(msg);
        callbackInvoked_ = true;
    }
};

TEST_F(ErrorHandlerTest, SafeExecuteReturnsTrue_WhenNoExceptionThrown) {
    bool executed = false;
    
    bool result = ErrorHandler::safeExecute([&]() {
        executed = true;
    }, "test operation");
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(executed);
}

TEST_F(ErrorHandlerTest, SafeExecuteReturnsFalse_WhenStdExceptionThrown) {
    bool result = ErrorHandler::safeExecute([]() {
        throw std::runtime_error("test error");
    }, "test operation");
    
    EXPECT_FALSE(result);
}

TEST_F(ErrorHandlerTest, SafeExecuteReturnsFalse_WhenUnknownExceptionThrown) {
    bool result = ErrorHandler::safeExecute([]() {
        throw 42; // Non-std exception
    }, "test operation");
    
    EXPECT_FALSE(result);
}

TEST_F(ErrorHandlerTest, SafeExecuteInvokesErrorCallback_WhenExceptionThrown) {
    auto callback = [this](const std::string& msg) {
        captureError(msg);
    };
    
    bool result = ErrorHandler::safeExecute([]() {
        throw std::runtime_error("test error");
    }, "test operation", callback);
    
    EXPECT_FALSE(result);
    EXPECT_TRUE(callbackInvoked_);
    EXPECT_EQ(errorMessages_.size(), 1);
    EXPECT_TRUE(errorMessages_[0].find("test operation") != std::string::npos);
    EXPECT_TRUE(errorMessages_[0].find("test error") != std::string::npos);
}

TEST_F(ErrorHandlerTest, SafeExecuteDoesNotInvokeErrorCallback_WhenNoException) {
    auto callback = [this](const std::string& msg) {
        captureError(msg);
    };
    
    bool result = ErrorHandler::safeExecute([]() {
        // No exception
    }, "test operation", callback);
    
    EXPECT_TRUE(result);
    EXPECT_FALSE(callbackInvoked_);
    EXPECT_EQ(errorMessages_.size(), 0);
}

TEST_F(ErrorHandlerTest, SafeExecuteWithReturnValue_ReturnsCorrectValue_WhenNoException) {
    int expectedValue = 42;
    
    int result = ErrorHandler::safeExecuteWithReturn<int>([]() {
        return 42;
    }, "test operation", -1);
    
    EXPECT_EQ(result, expectedValue);
}

TEST_F(ErrorHandlerTest, SafeExecuteWithReturnValue_ReturnsDefaultValue_WhenExceptionThrown) {
    int defaultValue = -1;
    
    int result = ErrorHandler::safeExecuteWithReturn<int>([]() {
        throw std::runtime_error("test error");
        return 42;
    }, "test operation", defaultValue);
    
    EXPECT_EQ(result, defaultValue);
}

TEST_F(ErrorHandlerTest, SafeExecuteHandlesComplexOperations) {
    std::vector<int> data;
    
    bool result = ErrorHandler::safeExecute([&data]() {
        data.push_back(1);
        data.push_back(2);
        data.push_back(3);
        // Simulate some complex operation
        if (data.size() != 3) {
            throw std::logic_error("Unexpected data size");
        }
    }, "complex operation");
    
    EXPECT_TRUE(result);
    EXPECT_EQ(data.size(), 3);
    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[1], 2);
    EXPECT_EQ(data[2], 3);
}

TEST_F(ErrorHandlerTest, ErrorMessageContainsContext) {
    auto callback = [this](const std::string& msg) {
        captureError(msg);
    };
    
    ErrorHandler::safeExecute([]() {
        throw std::invalid_argument("invalid input");
    }, "parsing configuration file", callback);
    
    EXPECT_EQ(errorMessages_.size(), 1);
    EXPECT_TRUE(errorMessages_[0].find("parsing configuration file") != std::string::npos);
    EXPECT_TRUE(errorMessages_[0].find("invalid input") != std::string::npos);
}

TEST_F(ErrorHandlerTest, MultipleErrorCallbacksWork) {
    std::vector<std::string> errors1, errors2;
    
    auto callback1 = [&errors1](const std::string& msg) {
        errors1.push_back(msg);
    };
    
    auto callback2 = [&errors2](const std::string& msg) {
        errors2.push_back(msg);
    };
    
    ErrorHandler::safeExecute([]() {
        throw std::runtime_error("error 1");
    }, "operation 1", callback1);
    
    ErrorHandler::safeExecute([]() {
        throw std::runtime_error("error 2");
    }, "operation 2", callback2);
    
    EXPECT_EQ(errors1.size(), 1);
    EXPECT_EQ(errors2.size(), 1);
    EXPECT_TRUE(errors1[0].find("error 1") != std::string::npos);
    EXPECT_TRUE(errors2[0].find("error 2") != std::string::npos);
}

}  // namespace test
}  // namespace Utils
}  // namespace ChipCarving
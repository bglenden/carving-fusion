/**
 * ErrorHandler.cpp
 *
 * Implementation of centralized error handling utility
 */

#include "ErrorHandler.h"
#include "DebugLogger.h"

#include <iostream>

namespace ChipCarving {
namespace Utils {

// Static member initialization
ErrorCallback ErrorHandler::globalErrorCallback_ = nullptr;
bool ErrorHandler::consoleLoggingEnabled_ = true;
bool ErrorHandler::userMessagesEnabled_ = false;

bool ErrorHandler::executeFusionOperation(
    const std::string& operation,
    std::function<bool()> func,
    bool showMessageToUser) {

    auto logger = DebugLogger::getInstance();
    logger->logDebug("Executing Fusion operation: " + operation);

    try {
        bool result = func();
        if (result) {
            logger->logDebug("Fusion operation succeeded: " + operation);
        } else {
            logger->logWarning("Fusion operation returned false: " + operation);
        }
        return result;
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception in " + operation + ": " + e.what();
        logger->logError(errorMsg);

        if (showMessageToUser && userMessagesEnabled_) {
            // TODO(dev): Show message to user via UI
            // For now, just log as error
            logger->logError("User should be notified: " + errorMsg);
        }

        if (globalErrorCallback_) {
            globalErrorCallback_(errorMsg, operation);
        }

        return false;
    } catch (...) {
        std::string errorMsg = "Unknown exception in " + operation;
        logger->logError(errorMsg);

        if (showMessageToUser && userMessagesEnabled_) {
            // TODO(dev): Show generic error message to user
            logger->logError("User should be notified: " + errorMsg);
        }

        if (globalErrorCallback_) {
            globalErrorCallback_(errorMsg, operation);
        }

        return false;
    }
}

void ErrorHandler::executeWithLogging(
    const std::string& operation,
    std::function<void()> func) {

    auto logger = DebugLogger::getInstance();
    logger->logDebug("Executing operation with logging: " + operation);

    try {
        func();
        logger->logDebug("Operation completed successfully: " + operation);
    } catch (const std::exception& e) {
        handleStandardException(operation, e);
    } catch (...) {
        handleUnknownException(operation);
    }
}

void ErrorHandler::setGlobalErrorCallback(ErrorCallback callback) {
    globalErrorCallback_ = callback;

    auto logger = DebugLogger::getInstance();
    if (callback) {
        logger->logInfo("Global error callback registered");
    } else {
        logger->logInfo("Global error callback cleared");
    }
}

void ErrorHandler::enableConsoleLogging(bool enabled) {
    consoleLoggingEnabled_ = enabled;

    auto logger = DebugLogger::getInstance();
    logger->logInfo("Console logging " + std::string(enabled ? "enabled" : "disabled"));
}

void ErrorHandler::enableUserMessages(bool enabled) {
    userMessagesEnabled_ = enabled;

    auto logger = DebugLogger::getInstance();
    logger->logInfo("User messages " + std::string(enabled ? "enabled" : "disabled"));
}

void ErrorHandler::handleStandardException(const std::string& operation, const std::exception& e) {
    std::string errorMsg = "Exception in " + operation + ": " + e.what();
    logError(operation, errorMsg);

    if (globalErrorCallback_) {
        globalErrorCallback_(errorMsg, operation);
    }
}

void ErrorHandler::handleUnknownException(const std::string& operation) {
    std::string errorMsg = "Unknown exception in " + operation;
    logError(operation, errorMsg);

    if (globalErrorCallback_) {
        globalErrorCallback_(errorMsg, operation);
    }
}

void ErrorHandler::logError(const std::string& operation, const std::string& error) {
    auto logger = DebugLogger::getInstance();
    logger->logError(error, operation);

    if (consoleLoggingEnabled_) {
        std::cerr << "[ErrorHandler] " << operation << ": " << error << std::endl;
    }
}

}  // namespace Utils
}  // namespace ChipCarving

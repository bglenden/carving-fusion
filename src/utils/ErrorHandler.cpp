/**
 * ErrorHandler.cpp
 *
 * Implementation of centralized error handling utility
 */

#include "ErrorHandler.h"

#include <iostream>

#include "adapters/IFusionInterface.h"
#include "utils/logging.h"

namespace ChipCarving {
namespace Utils {

// Static member initialization
ErrorCallback ErrorHandler::globalErrorCallback_ = nullptr;
bool ErrorHandler::consoleLoggingEnabled_ = true;
bool ErrorHandler::userMessagesEnabled_ = false;
Adapters::IUserInterface* ErrorHandler::userInterface_ = nullptr;

bool ErrorHandler::executeFusionOperation(const std::string& operation, std::function<bool()> func,
                                          bool showMessageToUser) {
  LOG_DEBUG("Executing Fusion operation: " << operation);

  try {
    bool result = func();
    if (result) {
      LOG_DEBUG("Fusion operation succeeded: " << operation);
    } else {
      LOG_WARNING("Fusion operation returned false: " << operation);
    }
    return result;
  } catch (const std::exception& e) {
    std::string errorMsg = "Exception in " + operation + ": " + e.what();
    LOG_ERROR(errorMsg);

    if (showMessageToUser && userMessagesEnabled_ && userInterface_) {
      userInterface_->showMessageBox("Error", errorMsg);
    }

    if (globalErrorCallback_) {
      globalErrorCallback_(errorMsg, operation);
    }

    return false;
  } catch (...) {
    std::string errorMsg = "Unknown exception in " + operation;
    LOG_ERROR(errorMsg);

    if (showMessageToUser && userMessagesEnabled_ && userInterface_) {
      userInterface_->showMessageBox("Error", errorMsg);
    }

    if (globalErrorCallback_) {
      globalErrorCallback_(errorMsg, operation);
    }

    return false;
  }
}

void ErrorHandler::executeWithLogging(const std::string& operation, std::function<void()> func) {
  LOG_DEBUG("Executing operation with logging: " << operation);

  try {
    func();
    LOG_DEBUG("Operation completed successfully: " << operation);
  } catch (const std::exception& e) {
    handleStandardException(operation, e);
  } catch (...) {
    handleUnknownException(operation);
  }
}

void ErrorHandler::setGlobalErrorCallback(ErrorCallback callback) {
  globalErrorCallback_ = callback;

  if (callback) {
    LOG_DEBUG("Global error callback registered");
  } else {
    LOG_DEBUG("Global error callback cleared");
  }
}

void ErrorHandler::enableConsoleLogging(bool enabled) {
  consoleLoggingEnabled_ = enabled;

  LOG_DEBUG("Console logging " << (enabled ? "enabled" : "disabled"));
}

void ErrorHandler::enableUserMessages(bool enabled) {
  userMessagesEnabled_ = enabled;

  LOG_DEBUG("User messages " << (enabled ? "enabled" : "disabled"));
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
  LOG_ERROR("[" << operation << "] " << error);

  if (consoleLoggingEnabled_) {
    LOG_ERROR("[ErrorHandler] " << operation << ": " << error);
  }
}

void ErrorHandler::setUserInterface(Adapters::IUserInterface* ui) {
  userInterface_ = ui;
  LOG_DEBUG("User interface " << (ui ? "set" : "cleared"));
}

}  // namespace Utils
}  // namespace ChipCarving

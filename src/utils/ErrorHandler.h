/**
 * ErrorHandler.h
 *
 * Centralized error handling utility to standardize exception handling
 * Eliminates 12+ instances of repetitive try/catch patterns
 */

#pragma once

#include <exception>
#include <functional>
#include <string>

namespace ChipCarving {
namespace Utils {

/**
 * Callback function type for error handling
 * Parameters: errorMessage, errorContext
 */
using ErrorCallback =
    std::function<void(const std::string&, const std::string&)>;

/**
 * Result wrapper for operations that can fail
 */
template <typename T>
class Result {
 public:
  static Result<T> success(T value) {
    return Result<T>(std::move(value), true, "");
  }

  static Result<T> failure(const std::string& error) {
    return Result<T>(T{}, false, error);
  }

  bool isSuccess() const { return success_; }
  bool hasError() const { return !success_; }

  const T& getValue() const { return value_; }
  const std::string& getError() const { return error_; }

 private:
  Result(T value, bool success, const std::string& error)
      : value_(std::move(value)), success_(success), error_(error) {}

  T value_;
  bool success_;
  std::string error_;
};

/**
 * Centralized error handler for common exception patterns
 *
 * Usage:
 *   auto result = ErrorHandler::executeWithHandling<bool>("operation", []() {
 *       return performRiskyOperation();
 *   });
 *
 *   if (result.hasError()) {
 *       // Handle error
 *   }
 */
class ErrorHandler {
 public:
  // Execute function with standard error handling
  template <typename T>
  static Result<T> executeWithHandling(const std::string& operation,
                                       std::function<T()> func,
                                       ErrorCallback errorCallback = nullptr);

  // Execute function with custom error handling
  template <typename T>
  static Result<T> executeWithCustomHandling(
      const std::string& operation, std::function<T()> func,
      std::function<void(const std::exception&)> stdExceptionHandler,
      std::function<void()> unknownExceptionHandler);

  // Fusion 360 specific error handling (returns false on error)
  static bool executeFusionOperation(const std::string& operation,
                                     std::function<bool()> func,
                                     bool showMessageToUser = false);

  // Void operations with error logging only
  static void executeWithLogging(const std::string& operation,
                                 std::function<void()> func);

  // Configuration
  static void setGlobalErrorCallback(ErrorCallback callback);
  static void enableConsoleLogging(bool enabled);
  static void enableUserMessages(bool enabled);

 private:
  static ErrorCallback globalErrorCallback_;
  static bool consoleLoggingEnabled_;
  static bool userMessagesEnabled_;

  static void handleStandardException(const std::string& operation,
                                      const std::exception& e);
  static void handleUnknownException(const std::string& operation);
  static void logError(const std::string& operation, const std::string& error);
};

// Template implementations
template <typename T>
Result<T> ErrorHandler::executeWithHandling(const std::string& operation,
                                            std::function<T()> func,
                                            ErrorCallback errorCallback) {
  try {
    T result = func();
    return Result<T>::success(result);
  } catch (const std::exception& e) {
    std::string errorMsg = "Exception in " + operation + ": " + e.what();
    logError(operation, errorMsg);

    if (errorCallback) {
      errorCallback(errorMsg, operation);
    } else if (globalErrorCallback_) {
      globalErrorCallback_(errorMsg, operation);
    }

    return Result<T>::failure(errorMsg);
  } catch (...) {
    std::string errorMsg = "Unknown exception in " + operation;
    logError(operation, errorMsg);

    if (errorCallback) {
      errorCallback(errorMsg, operation);
    } else if (globalErrorCallback_) {
      globalErrorCallback_(errorMsg, operation);
    }

    return Result<T>::failure(errorMsg);
  }
}

template <typename T>
Result<T> ErrorHandler::executeWithCustomHandling(
    const std::string& operation, std::function<T()> func,
    std::function<void(const std::exception&)> stdExceptionHandler,
    std::function<void()> unknownExceptionHandler) {
  try {
    T result = func();
    return Result<T>::success(result);
  } catch (const std::exception& e) {
    if (stdExceptionHandler) {
      stdExceptionHandler(e);
    } else {
      handleStandardException(operation, e);
    }
    return Result<T>::failure(e.what());
  } catch (...) {
    if (unknownExceptionHandler) {
      unknownExceptionHandler();
    } else {
      handleUnknownException(operation);
    }
    return Result<T>::failure("Unknown exception in " + operation);
  }
}

}  // namespace Utils
}  // namespace ChipCarving

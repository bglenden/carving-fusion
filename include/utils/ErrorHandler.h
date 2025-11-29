/**
 * ErrorHandler utility for consistent error handling patterns
 * Provides standardized exception handling and logging
 */

#pragma once

#include <functional>
#include <iostream>
#include <string>

namespace ChipCarving {
namespace Utils {

/**
 * Callback type for UI notification of errors
 * @param errorMessage The error message to display
 * @param operation Context of where the error occurred
 */
using UINotificationCallback = std::function<void(const std::string& errorMessage, const std::string& operation)>;

/**
 * Utility class for standardized error handling patterns
 * Reduces duplication in try-catch blocks throughout the codebase
 */
class ErrorHandler {
 public:
  /**
   * Execute a function with standard exception handling
   * Logs errors but continues execution
   *
   * @param operation Function to execute
   * @param context Description of the operation for error messages
   * @param onError Optional callback for custom error handling
   * @return true if successful, false if exception occurred
   */
  template <typename Func>
  static bool safeExecute(Func&& operation, const std::string& context,
                          std::function<void(const std::string&)> onError = nullptr) {
    try {
      operation();
      return true;
    } catch (const std::exception& e) {
      std::string errorMsg = context + ": " + e.what();
      std::cerr << "ERROR: " << errorMsg << std::endl;
      if (onError) {
        onError(errorMsg);
      }
      return false;
    } catch (...) {
      std::string errorMsg = context + ": unknown exception";
      std::cerr << "ERROR: " << errorMsg << std::endl;
      if (onError) {
        onError(errorMsg);
      }
      return false;
    }
  }

  /**
   * Execute a function with exception handling for plugin operations
   * Designed to prevent crashes in Fusion 360 plugin context
   *
   * @param operation Function to execute
   * @param context Description of the operation
   * @param defaultValue Value to return on error
   * @return Result of operation or defaultValue on error
   */
  template <typename Func, typename ReturnType>
  static ReturnType pluginSafeExecute(Func&& operation, const std::string& context, ReturnType defaultValue) {
    try {
      return operation();
    } catch (const std::exception& e) {
      std::cerr << "Plugin error in " << context << ": " << e.what() << std::endl;
      return defaultValue;
    } catch (...) {
      std::cerr << "Unknown plugin error in " << context << std::endl;
      return defaultValue;
    }
  }

  /**
   * Execute a function with Fusion 360-specific error handling
   * Shows user-facing messages and logs appropriately
   *
   * @param operation Name of the operation for logging
   * @param func Function to execute (must return bool)
   * @param showMessageToUser Whether to show error messages to the user
   * @return Result of the function execution
   */
  static bool executeFusionOperation(const std::string& operation, std::function<bool()> func,
                                     bool showMessageToUser = false);

  /**
   * Execute optional metadata parsing with graceful failure
   * Used for parsing optional JSON fields that may be missing
   *
   * @param parser Function that may throw on missing/invalid data
   * @param fieldName Name of the field being parsed
   */
  template <typename Func>
  static void parseOptionalField(Func&& parser, const std::string& fieldName) {
    try {
      parser();
    } catch (const std::exception& e) {
      // Optional field parsing failure - continue silently
      // Could add debug logging here if needed
    }
  }

  /**
   * Set the UI notification callback for user-facing error messages
   * @param callback Function to call when user notification needed
   */
  static void setUINotificationCallback(UINotificationCallback callback) {
    uiNotificationCallback_ = callback;
  }

  /**
   * Enable or disable UI notifications
   * @param enabled Whether to show UI notifications
   */
  static void enableUINotifications(bool enabled) { uiNotificationsEnabled_ = enabled; }

  /**
   * Check if UI notifications are enabled
   * @return true if UI notifications enabled
   */
  static bool uiNotificationsEnabled() { return uiNotificationsEnabled_; }

 private:
  static UINotificationCallback uiNotificationCallback_;
  static bool uiNotificationsEnabled_;
};

}  // namespace Utils
}  // namespace ChipCarving

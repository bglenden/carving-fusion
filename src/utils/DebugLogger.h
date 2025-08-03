/**
 * DebugLogger.h
 *
 * Centralized debug logging utility to eliminate code duplication
 * Replaces 31+ instances of manual TempFileManager::getLogFilePath usage
 */

#pragma once

#include <memory>
#include <string>

namespace ChipCarving {
namespace Utils {

/**
 * Levels for debug logging
 */
enum class LogLevel { LOG_DEBUG, INFO, WARNING, ERROR };

/**
 * Centralized debug logger that abstracts file operations and formatting
 *
 * Usage:
 *   auto logger = DebugLogger::getInstance();
 *   logger->logInfo("Method called", "extractProfileVertices");
 *   logger->logError("No application instance");
 *   logger->logSectionHeader("PROFILE EXTRACTION", "extractProfileVertices
 * called");
 */
class DebugLogger {
 public:
  // Singleton pattern for global access
  static std::shared_ptr<DebugLogger> getInstance();

  // Main logging methods
  void logInfo(const std::string& message, const std::string& context = "");
  void logDebug(const std::string& message, const std::string& context = "");
  void logWarning(const std::string& message, const std::string& context = "");
  void logError(const std::string& message, const std::string& context = "");

  // Special formatting methods
  void logSectionHeader(const std::string& section,
                        const std::string& details = "");
  void logMethodEntry(const std::string& methodName,
                      const std::string& parameters = "");
  void logMethodExit(const std::string& methodName,
                     const std::string& result = "");

  // Indicator file support (for critical debugging)
  void createIndicatorFile(const std::string& filename,
                           const std::string& content);

  // Configuration
  void setLogFile(const std::string& logFileName);
  void enableAutoFlush(bool enabled);

 private:
  DebugLogger();

  void logMessage(LogLevel level, const std::string& message,
                  const std::string& context = "");
  std::string getLevelPrefix(LogLevel level) const;
  std::string getCurrentTimestamp() const;

  static std::shared_ptr<DebugLogger> instance_;
  std::string logFileName_;
  bool autoFlush_;
};

}  // namespace Utils
}  // namespace ChipCarving

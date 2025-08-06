#ifndef INCLUDE_UTILS_LOGGING_H_
#define INCLUDE_UTILS_LOGGING_H_

#include <sstream>
#include <string>

/**
 * Log levels for the plugin
 */
enum class LogLevel { LOG_DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3 };

/**
 * Log a message to the Fusion 360 Text Commands window
 */
void LogToConsole(const std::string& message);

/**
 * Log a message with a specific log level
 */
void LogToConsole(LogLevel level, const std::string& message);

/**
 * Set the minimum log level for output
 */
void SetMinLogLevel(LogLevel level);

/**
 * Get the current minimum log level
 */
LogLevel GetMinLogLevel();

// Conditional debug logging macros
#ifdef DEBUG
#define LOG_DEBUG(msg)                                                                 \
  do {                                                                                 \
    if (static_cast<int>(LogLevel::LOG_DEBUG) >= static_cast<int>(GetMinLogLevel())) { \
      std::ostringstream _debug_stream;                                                \
      _debug_stream << msg;                                                            \
      LogToConsole(LogLevel::LOG_DEBUG, _debug_stream.str());                          \
    }                                                                                  \
  } while (0)

#define LOG_DEBUG_ENTRY(method) LOG_DEBUG(method << " called")

#define LOG_DEBUG_EXIT(method) LOG_DEBUG(method << " completed")

#define LOG_DEBUG_VALUE(name, value) LOG_DEBUG(name << " = " << value)
#else
#define LOG_DEBUG(msg) \
  do {                 \
  } while (0)
#define LOG_DEBUG_ENTRY(method) \
  do {                          \
  } while (0)
#define LOG_DEBUG_EXIT(method) \
  do {                         \
  } while (0)
#define LOG_DEBUG_VALUE(name, value) \
  do {                               \
  } while (0)
#endif

// Always-enabled logging macros with short-circuit optimization
#define LOG_INFO(msg)                                                             \
  do {                                                                            \
    if (static_cast<int>(LogLevel::INFO) >= static_cast<int>(GetMinLogLevel())) { \
      std::ostringstream _info_stream;                                            \
      _info_stream << msg;                                                        \
      LogToConsole(LogLevel::INFO, _info_stream.str());                           \
    }                                                                             \
  } while (0)

#define LOG_WARNING(msg)                                                             \
  do {                                                                               \
    if (static_cast<int>(LogLevel::WARNING) >= static_cast<int>(GetMinLogLevel())) { \
      std::ostringstream _warn_stream;                                               \
      _warn_stream << msg;                                                           \
      LogToConsole(LogLevel::WARNING, _warn_stream.str());                           \
    }                                                                                \
  } while (0)

#define LOG_ERROR(msg)                                                             \
  do {                                                                             \
    if (static_cast<int>(LogLevel::ERROR) >= static_cast<int>(GetMinLogLevel())) { \
      std::ostringstream _error_stream;                                            \
      _error_stream << msg;                                                        \
      LogToConsole(LogLevel::ERROR, _error_stream.str());                          \
    }                                                                              \
  } while (0)

#endif  // INCLUDE_UTILS_LOGGING_H_

#include "../../include/utils/logging.h"

#include <Core/CoreAll.h>

#include <chrono>
#include <iomanip>
#include <sstream>

using namespace adsk::core;

// Global minimum log level (default to WARNING for cleaner output)
static LogLevel g_minLogLevel = LogLevel::WARNING;

void LogToConsole(const std::string& message) {
  LogToConsole(LogLevel::INFO, message);
}

void LogToConsole(LogLevel level, const std::string& message) {
  try {
    // Check if message should be logged based on level
    if (static_cast<int>(level) < static_cast<int>(g_minLogLevel)) {
      return;
    }

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    // Format timestamp
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

    // Get level prefix
    std::string levelPrefix;
    switch (level) {
      case LogLevel::LOG_DEBUG:
        levelPrefix = "[DEBUG]";
        break;
      case LogLevel::INFO:
        levelPrefix = "[INFO]";
        break;
      case LogLevel::WARNING:
        levelPrefix = "[WARN]";
        break;
      case LogLevel::ERROR:
        levelPrefix = "[ERROR]";
        break;
    }

    // Create log message
    std::string logMessage = "[" + ss.str() + "] " + levelPrefix + " " + message;

    // Get the application and UI
    Ptr<Application> app = Application::get();
    if (!app)
      return;

    Ptr<UserInterface> ui = app->userInterface();
    if (!ui)
      return;

    // Try to write to Text Commands palette
    Ptr<Palette> textPalette = ui->palettes()->itemById("TextCommands");
    if (textPalette) {
      Ptr<TextCommandPalette> textCommandPalette = textPalette;
      textCommandPalette->writeText(logMessage);
    }
  } catch (...) {
    // Silently fail if logging doesn't work
  }
}

void SetMinLogLevel(LogLevel level) {
  g_minLogLevel = level;
}

LogLevel GetMinLogLevel() {
  return g_minLogLevel;
}

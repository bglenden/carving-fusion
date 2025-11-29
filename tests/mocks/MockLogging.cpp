/**
 * MockLogging.cpp
 * 
 * Mock implementation of logging functions for unit tests
 * Avoids dependency on Fusion 360 SDK in test environment
 */

#include "utils/logging.h"
#include <iostream>

// Global minimum log level for tests
static LogLevel g_minLogLevel = LogLevel::LOG_DEBUG;

void LogToConsole(const std::string& message) {
    // In tests, just output to stdout
    std::cout << "[TEST] " << message << std::endl;
}

void LogToConsole(LogLevel level, const std::string& message) {
    // Check if message should be logged based on level
    if (static_cast<int>(level) < static_cast<int>(g_minLogLevel)) {
        return;
    }
    
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
    
    // Output to stdout for tests
    std::cout << "[TEST] " << levelPrefix << " " << message << std::endl;
}

void SetMinLogLevel(LogLevel level) {
    g_minLogLevel = level;
}

LogLevel GetMinLogLevel() {
    return g_minLogLevel;
}
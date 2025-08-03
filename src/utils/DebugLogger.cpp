/**
 * DebugLogger.cpp
 *
 * Implementation of centralized debug logging utility
 */

#include "DebugLogger.h"
#include "../../include/utils/TempFileManager.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace ChipCarving {
namespace Utils {

std::shared_ptr<DebugLogger> DebugLogger::instance_ = nullptr;

std::shared_ptr<DebugLogger> DebugLogger::getInstance() {
    if (!instance_) {
        instance_ = std::shared_ptr<DebugLogger>(new DebugLogger());
    }
    return instance_;
}

DebugLogger::DebugLogger()
    : logFileName_("fusion_cpp_debug.log"), autoFlush_(true) {
}

void DebugLogger::logInfo(const std::string& message, const std::string& context) {
    logMessage(LogLevel::INFO, message, context);
}

void DebugLogger::logDebug(const std::string& message, const std::string& context) {
    logMessage(LogLevel::LOG_DEBUG, message, context);
}

void DebugLogger::logWarning(const std::string& message, const std::string& context) {
    logMessage(LogLevel::WARNING, message, context);
}

void DebugLogger::logError(const std::string& message, const std::string& context) {
    logMessage(LogLevel::ERROR, message, context);
}

void DebugLogger::logSectionHeader(const std::string& section, const std::string& details) {
    std::ostringstream oss;
    oss << "=== " << section;
    if (!details.empty()) {
        oss << ": " << details;
    }
    oss << " ===";

    logMessage(LogLevel::INFO, oss.str());
}

void DebugLogger::logMethodEntry(const std::string& methodName, const std::string& parameters) {
    std::ostringstream oss;
    oss << "ENTER " << methodName;
    if (!parameters.empty()) {
        oss << "(" << parameters << ")";
    }

    logMessage(LogLevel::LOG_DEBUG, oss.str());
}

void DebugLogger::logMethodExit(const std::string& methodName, const std::string& result) {
    std::ostringstream oss;
    oss << "EXIT " << methodName;
    if (!result.empty()) {
        oss << " -> " << result;
    }

    logMessage(LogLevel::LOG_DEBUG, oss.str());
}

void DebugLogger::createIndicatorFile(const std::string& filename, const std::string& content) {
    try {
        std::string indicatorPath = chip_carving::TempFileManager::getLogFilePath(filename);
        std::ofstream indicator(indicatorPath);
        if (indicator.is_open()) {
            indicator << content << std::endl;
            indicator << "Created at: " << getCurrentTimestamp() << std::endl;
            indicator.close();
        }
    } catch (...) {
        // Don't let indicator file creation failure break the main logic
        logError("Failed to create indicator file: " + filename);
    }
}

void DebugLogger::setLogFile(const std::string& logFileName) {
    logFileName_ = logFileName;
}

void DebugLogger::enableAutoFlush(bool enabled) {
    autoFlush_ = enabled;
}

void DebugLogger::logMessage(LogLevel level, const std::string& message, const std::string& context) {
    try {
        std::string logPath = chip_carving::TempFileManager::getLogFilePath(logFileName_);
        std::ofstream logFile(logPath, std::ios::app);

        if (logFile.is_open()) {
            // Format: [TIMESTAMP] [LEVEL] context: message
            logFile << "[" << getCurrentTimestamp() << "] "
                    << getLevelPrefix(level);

            if (!context.empty()) {
                logFile << " " << context << ": ";
            } else {
                logFile << " ";
            }

            logFile << message << std::endl;

            if (autoFlush_) {
                logFile.flush();
            }

            logFile.close();
        }
    } catch (...) {
        // If logging fails, at least try to output to console
        std::cerr << "DebugLogger failed to write: " << message << std::endl;
    }
}

std::string DebugLogger::getLevelPrefix(LogLevel level) const {
    switch (level) {
        case LogLevel::LOG_DEBUG:   return "[DEBUG]";
        case LogLevel::INFO:        return "[INFO]";
        case LogLevel::WARNING:     return "[WARNING]";
        case LogLevel::ERROR:       return "[ERROR]";
        default:                    return "[UNKNOWN]";
    }
}

std::string DebugLogger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

}  // namespace Utils
}  // namespace ChipCarving

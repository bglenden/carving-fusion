/**
 * FusionLogger.cpp
 *
 * Concrete implementation of ILogger interface for Fusion 360
 */

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "FusionAPIAdapter.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

using namespace ChipCarving::Adapters;

// FusionLogger Implementation
FusionLogger::FusionLogger(const std::string& logFilePath)
    : logFilePath_(logFilePath) {
  // Log rotation: Move existing log to backup before creating new one
  rotateLogFile();

  // Create fresh log file for this session
  logFile_.open(logFilePath_, std::ios::out | std::ios::trunc);

  // Write session header
  if (logFile_.is_open()) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    logFile_ << "========================================" << std::endl;
    logFile_ << "NEW FUSION PLUGIN SESSION STARTED" << std::endl;
    logFile_ << "Timestamp: "
             << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
             << std::endl;
    logFile_ << "Log file: " << logFilePath_ << std::endl;
    logFile_ << "========================================" << std::endl;
    logFile_.flush();
  }
}

FusionLogger::~FusionLogger() {
  if (logFile_.is_open()) {
    logFile_ << "========================================" << std::endl;
    logFile_ << "SESSION ENDED" << std::endl;
    logFile_ << "========================================" << std::endl;
    logFile_.close();
  }
}

void FusionLogger::logInfo(const std::string& message) const {
  writeLog(message, "INFO");
}

void FusionLogger::logDebug(const std::string& message) const {
  writeLog(message, "DEBUG");
}

void FusionLogger::logWarning(const std::string& message) const {
  writeLog(message, "WARNING");
}

void FusionLogger::logError(const std::string& message) const {
  writeLog(message, "ERROR");
}

void FusionLogger::writeLog(const std::string& message,
                            const std::string& level) const {
  // Check and rotate log file if needed before writing
  const_cast<FusionLogger*>(this)->checkAndRotateIfNeeded();

  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream timestampedMessage;
  timestampedMessage << "["
                     << std::put_time(std::localtime(&time_t), "%H:%M:%S")
                     << "] "
                     << "[" << level << "] " << message;

  std::string fullMessage = timestampedMessage.str();

  // Write to file with error handling
  if (logFile_.is_open()) {
    logFile_ << fullMessage << std::endl;
    logFile_.flush();  // Ensure immediate write

    // Check for write errors
    if (logFile_.fail()) {
      // Try to reopen the file
      logFile_.close();
      logFile_.open(logFilePath_, std::ios::out | std::ios::app);
      if (logFile_.is_open()) {
        logFile_ << "[ERROR] Log file write failed, reopened file" << std::endl;
        logFile_ << fullMessage << std::endl;
      }
    }
  } else {
    // Try to open the file if it's not open
    logFile_.open(logFilePath_, std::ios::out | std::ios::app);
    if (logFile_.is_open()) {
      logFile_ << "[ERROR] Log file was closed, reopened" << std::endl;
      logFile_ << fullMessage << std::endl;
    }
  }

  // Console output disabled for performance in production
  // std::cout << fullMessage << std::endl;
}

void FusionLogger::rotateLogFile() {
  // Check if log file exists
  std::ifstream existingFile(logFilePath_);
  if (existingFile.good()) {
    // Check file size
    existingFile.seekg(0, std::ios::end);
    size_t fileSize = existingFile.tellg();
    existingFile.close();

    // Always rotate on new session, or if file is larger than 1MB
    const size_t MAX_LOG_SIZE = 1 * 1024 * 1024;  // 1MB
    (void)MAX_LOG_SIZE;  // Suppress unused variable warning (reserved for
                         // future use)
    bool shouldRotate = (fileSize > 0);  // Always rotate on startup for now

    if (shouldRotate) {
      // Create backup filename with timestamp
      auto now = std::chrono::system_clock::now();
      auto time_t = std::chrono::system_clock::to_time_t(now);

      std::stringstream backupName;
      backupName << logFilePath_ << ".backup_"
                 << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");

      // Rename existing file to backup
      std::rename(logFilePath_.c_str(), backupName.str().c_str());

      // For now, don't clean up backups - let user manage them
      // TODO(refactor): Add backup cleanup logic if needed
    }
  }
}

void FusionLogger::checkAndRotateIfNeeded() {
  if (logFile_.is_open()) {
    // Check current file size
    size_t currentPos = logFile_.tellp();
    const size_t MAX_LOG_SIZE = 5 * 1024 * 1024;  // 5MB during runtime

    if (currentPos > MAX_LOG_SIZE) {
      // Close current file
      logFile_.close();

      // Rotate the file
      rotateLogFile();

      // Open new file
      logFile_.open(logFilePath_, std::ios::out | std::ios::app);
    }
  }
}

// FusionUserInterface Implementation
FusionUserInterface::FusionUserInterface(Ptr<UserInterface> ui) : ui_(ui) {}

void FusionUserInterface::showMessageBox(const std::string& title,
                                         const std::string& message) {
  if (ui_) {
    ui_->messageBox(message, title);
  }
}

std::string FusionUserInterface::showFileDialog(const std::string& title,
                                                const std::string& filter) {
  if (!ui_) {
    return "";
  }

  try {
    // Create file dialog
    Ptr<FileDialog> fileDialog = ui_->createFileDialog();
    if (!fileDialog) {
      return "";
    }

    // Configure the dialog
    fileDialog->isMultiSelectEnabled(false);
    fileDialog->title(title);
    fileDialog->filter(filter);

    // Show the dialog
    DialogResults result = fileDialog->showOpen();
    if (result == DialogOK) {
      if (fileDialog->filenames().size() > 0) {
        return fileDialog->filename();
      }
    }

  } catch (const std::exception& e) {
    // Log error but don't crash
    std::cout << "File dialog error: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "Unknown file dialog error" << std::endl;
  }

  return "";
}

std::string FusionUserInterface::selectJsonFile() {
  return showFileDialog("Select JSON File", "JSON Files (*.json)");
}

bool FusionUserInterface::confirmAction(const std::string& message) {
  // TODO(ui): Implement confirmation dialog
  // For now, return true
  (void)message;  // Suppress warning
  return true;
}

bool FusionUserInterface::showParameterDialog(const std::string& title,
                                              MedialAxisParameters& params) {
  if (!ui_) {
    return false;
  }

  // For now, implement as simple message box - TODO(ui): Replace with proper
  // dialog
  std::string message = "Medial Axis Parameters\n\n";
  message +=
      "Polygon Tolerance: " + std::to_string(params.polygonTolerance) + "mm\n";
  message +=
      "Sampling Distance: " + std::to_string(params.samplingDistance) + "mm\n";
  message += "Force Boundary Intersections: ";
  message += (params.forceBoundaryIntersections ? "Yes" : "No");
  message += "\n";
  message += "Show Medial Lines: ";
  message += (params.showMedialLines ? "Yes" : "No");
  message += "\n";
  message += "Show Clearance Circles: ";
  message += (params.showClearanceCircles ? "Yes" : "No");
  message += "\n";
  message += "Show Polygonized Shape: ";
  message += (params.showPolygonizedShape ? "Yes" : "No");
  message += "\n\n";
  message += "Using default parameters for now.";

  ui_->messageBox(message, title);
  return true;  // Always proceed with defaults for now
}

SketchSelection FusionUserInterface::showSketchSelectionDialog(
    const std::string& title) {
  SketchSelection selection;

  if (!ui_) {
    selection.errorMessage = "UI not available";
    return selection;
  }

  // For now, implement as placeholder - TODO(ui): Replace with proper selection
  // dialog
  std::string message = "Sketch Selection\n\n";
  message += "This dialog will allow selection of closed sketch paths.\n";
  message += "For now, proceeding without selection.\n\n";
  message += "TODO(ui): Implement proper sketch selection UI";

  ui_->messageBox(message, title);

  selection.isValid = false;
  selection.errorMessage = "Sketch selection not yet implemented";
  return selection;
}

void FusionUserInterface::updateSelectionCount(int count) {
  // TODO(ui): Update selection count in UI
  // For now, this is a no-op
  (void)count;  // Suppress warning
}

}  // namespace Adapters
}  // namespace ChipCarving

/**
 * MockLogger.h
 * Mock logger for testing - captures log messages for verification
 */

#pragma once

#include <string>
#include <vector>

#include "adapters/IFusionInterface.h"

using namespace ChipCarving::Adapters;

class MockLogger : public ILogger {
 public:
  void logInfo(const std::string& message) const override { infoMessages.push_back(message); }

  void logDebug(const std::string& message) const override { debugMessages.push_back(message); }

  void logWarning(const std::string& message) const override { warningMessages.push_back(message); }

  void logError(const std::string& message) const override { errorMessages.push_back(message); }

  // Test helpers (mutable to allow const methods to modify)
  mutable std::vector<std::string> infoMessages;
  mutable std::vector<std::string> debugMessages;
  mutable std::vector<std::string> warningMessages;
  mutable std::vector<std::string> errorMessages;

  void clearMessages() {
    infoMessages.clear();
    debugMessages.clear();
    warningMessages.clear();
    errorMessages.clear();
  }
};

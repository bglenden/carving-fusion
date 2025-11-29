/**
 * PluginManagerUtils.cpp
 *
 * Utility methods and helpers for PluginManager
 * Split from PluginManager.cpp for maintainability
 */

#include <algorithm>
#include <fstream>
#include <sstream>

#include "PluginManager.h"

namespace ChipCarving {
namespace Core {

void PluginManager::logStartup() {
  if (logger_) {
    logger_->logInfo("PluginManager startup completed successfully");
  }
}

void PluginManager::logShutdown() {
  if (logger_) {
    logger_->logInfo("PluginManager shutdown completed successfully");
  }
}

std::string PluginManager::formatMedialAxisResults(const Geometry::MedialAxisResults& results) {
  std::string formatted = "Chains: " + std::to_string(results.numChains) +
                          ", Points: " + std::to_string(results.totalPoints) +
                          ", Length: " + std::to_string(static_cast<int>(results.totalLength)) + "mm";

  if (results.totalPoints > 0) {
    formatted +=
        ", Clearance: " + std::to_string(results.minClearance) + "-" + std::to_string(results.maxClearance) + "mm";
  }

  return formatted;
}

}  // namespace Core
}  // namespace ChipCarving

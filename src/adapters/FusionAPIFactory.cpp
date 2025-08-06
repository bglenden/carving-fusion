/**
 * FusionAPIFactory.cpp
 *
 * Factory class for creating Fusion 360 API adapter instances
 */

#include "../../include/utils/logging.h"
#include "FusionAPIAdapter.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

// FusionAPIFactory Implementation
FusionAPIFactory::FusionAPIFactory(Ptr<Application> app, Ptr<UserInterface> ui, const std::string& logFilePath)
    : app_(app), ui_(ui), logFilePath_(logFilePath) {}

std::unique_ptr<ILogger> FusionAPIFactory::createLogger() {
  return std::make_unique<FusionLogger>(logFilePath_);
}

std::unique_ptr<IUserInterface> FusionAPIFactory::createUserInterface() {
  return std::make_unique<FusionUserInterface>(ui_);
}

std::unique_ptr<IWorkspace> FusionAPIFactory::createWorkspace() {
  // Debug: Log workspace creation
  LOG_DEBUG("[FACTORY] Creating FusionWorkspace instance");
  auto workspace = std::make_unique<FusionWorkspace>(app_);
  LOG_DEBUG("[FACTORY] FusionWorkspace created successfully");
  return workspace;
}

}  // namespace Adapters
}  // namespace ChipCarving

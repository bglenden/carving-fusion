/**
 * FusionAPIFactory.cpp
 *
 * Factory class for creating Fusion 360 API adapter instances
 */

#include <fstream>

#include "FusionAPIAdapter.h"
#include "../../include/utils/TempFileManager.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

// FusionAPIFactory Implementation
FusionAPIFactory::FusionAPIFactory(Ptr<Application> app, Ptr<UserInterface> ui,
                                   const std::string& logFilePath)
    : app_(app), ui_(ui), logFilePath_(logFilePath) {}

std::unique_ptr<ILogger> FusionAPIFactory::createLogger() {
    return std::make_unique<FusionLogger>(logFilePath_);
}

std::unique_ptr<IUserInterface> FusionAPIFactory::createUserInterface() {
    return std::make_unique<FusionUserInterface>(ui_);
}

std::unique_ptr<IWorkspace> FusionAPIFactory::createWorkspace() {
    // Debug: Log workspace creation
    std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("fusion_cpp_debug.log");
    std::ofstream debugLog(debugLogPath, std::ios::app);
    if (debugLog.is_open()) {
        debugLog << "[FACTORY] Creating FusionWorkspace instance" << std::endl;
        debugLog.flush();
    }
    auto workspace = std::make_unique<FusionWorkspace>(app_);
    if (debugLog.is_open()) {
        debugLog << "[FACTORY] FusionWorkspace created successfully" << std::endl;
        debugLog.flush();
    }
    return workspace;
}

}  // namespace Adapters
}  // namespace ChipCarving

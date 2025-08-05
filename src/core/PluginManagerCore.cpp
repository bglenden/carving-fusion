/**
 * PluginManagerCore.cpp
 *
 * Core lifecycle operations for PluginManager
 * Split from PluginManager.cpp for maintainability
 */

#include "PluginManager.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>

#include "../../include/geometry/Point2D.h"
#include "../../include/geometry/Point3D.h"
#include "../../include/geometry/VCarveCalculator.h"
#include "../../include/parsers/DesignParser.h"
#include "../../include/utils/TempFileManager.h"
#include "../utils/UnitConversion.h"
#include "../version.h"

namespace ChipCarving {
namespace Core {

PluginManager::PluginManager(std::unique_ptr<Adapters::IFusionFactory> factory)
    : factory_(std::move(factory)), initialized_(false) {}

bool PluginManager::initialize() {
    if (initialized_) {
        return true;
    }

    try {
        // Create dependencies through factory
        logger_ = factory_->createLogger();
        ui_ = factory_->createUserInterface();

        // Debug: Log workspace creation
        workspace_ = factory_->createWorkspace();

        if (workspace_) {
        } else {
        }

        if (!logger_ || !ui_ || !workspace_) {
            return false;
        }

        // Initialize MedialAxisProcessor with default parameters
        medialProcessor_ = std::make_unique<Geometry::MedialAxisProcessor>(0.25, 0.8);
        medialProcessor_->setVerbose(true);  // Enable verbose logging to debug crash

        // Clear debug logs on startup for fresh debugging
        std::string debugLogPath = chip_carving::TempFileManager::getLogFilePath("medial_axis_debug.log");
        std::string immediateLogPath = chip_carving::TempFileManager::getLogFilePath("medial_immediate.log");
        std::ofstream clearLog1(debugLogPath, std::ios::out);
        clearLog1.close();
        std::ofstream clearLog2(immediateLogPath, std::ios::out);
        clearLog2.close();

        logStartup();
        initialized_ = true;

        return true;

    } catch (const std::exception& e) {
        if (logger_) {
            // TODO(developer): Log exception: e.what()
        }
        return false;
    } catch (...) {
        if (logger_) {
            // TODO(developer): Log unknown exception
        }
        return false;
    }
}

void PluginManager::shutdown() {
    if (!initialized_) {
        return;
    }

    try {
        logShutdown();

        // Clean up resources
        workspace_.reset();
        ui_.reset();
        logger_.reset();
        factory_.reset();

        initialized_ = false;

    } catch (const std::exception& e) {
        if (logger_) {
            // TODO(developer): Log exception: e.what()
        }
    } catch (...) {
        if (logger_) {
            // TODO(developer): Log unknown exception
        }
    }
}

std::string PluginManager::getVersion() const {
    return ADDIN_VERSION_STRING;
}

std::string PluginManager::getName() const {
    return std::string(ADDIN_NAME) + " (Refactored)";
}

}  // namespace Core
}  // namespace ChipCarving

/**
 * PluginManagerCore.cpp
 *
 * Core lifecycle operations for PluginManager
 * Split from PluginManager.cpp for maintainability
 */

#include "PluginManager.h"

#include <algorithm>
#include <set>
#include <sstream>

#include "../../include/geometry/Point2D.h"
#include "../../include/geometry/Point3D.h"
#include "../../include/geometry/VCarveCalculator.h"
#include "../../include/parsers/DesignParser.h"
#include "../../include/utils/logging.h"
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

        // Log startup (file logs have been removed)

        logStartup();
        initialized_ = true;

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Exception during initialization: " << e.what());
        return false;
    } catch (...) {
        LOG_ERROR("Unknown exception during initialization");
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
        LOG_ERROR("Exception during initialization: " << e.what());
    } catch (...) {
        LOG_ERROR("Unknown exception during initialization");
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

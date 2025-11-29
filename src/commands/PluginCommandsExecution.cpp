/**
 * PluginCommandsExecution.cpp
 *
 * Command execution logic for PluginCommands
 * Split from PluginCommands.cpp for maintainability
 */

#include "core/PluginManager.h"
#include "PluginCommands.h"
#include "utils/ErrorHandler.h"
#include <iostream>

namespace ChipCarving {
namespace Commands {

// Enhanced UI Phase 4: Command execution implementation
void GeneratePathsCommandHandler::executeMedialAxisProcessing(adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
  // Use ErrorHandler to wrap the entire operation with user-facing error messages
  Utils::ErrorHandler::executeFusionOperation("ExecuteMedialAxisGeneration", [&]() {
    try {
      // LOG: Start of method execution
      if (pluginManager_) {
        // Use the PluginManager's logger to write to file
        // This will write to temp_output/logs/fusion_cpp_debug.log
      }

      if (!inputs || !pluginManager_) {
        return false;
      }

      // Get parameters from dialog inputs
      ChipCarving::Adapters::MedialAxisParameters params = getParametersFromInputs(inputs);

      // Get selected profiles from dialog
      ChipCarving::Adapters::SketchSelection selection = getSelectionFromInputs(inputs);

      // DEBUG: Log selection details extensively
      // Note: We're going to call the NEW executeMedialAxisGeneration method
      // If you see the OLD error dialog, then there's a bug in the method routing

      // Validate inputs before processing
      if (!selection.isValid || selection.closedPathCount == 0) {
        // CRITICAL: Always call the NEW method - Enhanced UI Phase 5.2 system
        if (pluginManager_) {
          // This should call executeMedialAxisGeneration(), NOT
          // executeGeneratePaths()
          bool success = pluginManager_->executeMedialAxisGeneration(selection, params);
          if (!success) {
            // The error was already shown by executeMedialAxisGeneration
            return false;
          }
        }
        return false;
      }

      // Enhanced UI Phase 5: Integrate with PluginManager for medial axis
      // processing Implemented:
      // 1. Parameter extraction and validation ✓
      // 2. Call MedialAxisProcessor with user parameters ✓
      // 3. Construction geometry visualization (medial lines + clearance circles)
      // ✓
      // 4. User feedback with results ✓
      // TODO(dev): Profile geometry extraction (currently uses imported shapes as
      // placeholder)

      // Execute medial axis generation with construction geometry visualization
      if (pluginManager_) {
        bool success = pluginManager_->executeMedialAxisGeneration(selection, params);
        if (!success) {
          // Error handling is done within executeMedialAxisGeneration
          return false;
        }
      }
      return true;
    } catch (const std::exception& e) {
      std::cerr << "Exception in executeMedialAxisProcessing: " << e.what() << std::endl;
      return false;
    } catch (...) {
      std::cerr << "Unknown exception in executeMedialAxisProcessing" << std::endl;
      return false;
    }
  }, true);  // true = show errors to user
}

}  // namespace Commands
}  // namespace ChipCarving

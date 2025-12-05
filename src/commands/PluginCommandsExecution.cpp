/**
 * PluginCommandsExecution.cpp
 *
 * Command execution logic for PluginCommands
 * Split from PluginCommands.cpp for maintainability
 */

#include "PluginCommands.h"
#include "core/PluginManager.h"  // IWYU pragma: keep
#include "utils/ErrorHandler.h"

namespace ChipCarving {
namespace Commands {

// Enhanced UI Phase 4: Command execution implementation
void GeneratePathsCommandHandler::executeMedialAxisProcessing(adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
  // Use ErrorHandler to wrap the entire operation with user-facing error messages
  Utils::ErrorHandler::executeFusionOperation(
      "ExecuteMedialAxisGeneration",
      [&]() {
        // LOG: Start of method execution
        if (pluginManager()) {
          // Use the PluginManager's logger to write to file
          // This will write to temp_output/logs/fusion_cpp_debug.log
        }

        if (!inputs || !pluginManager()) {
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
          if (pluginManager()) {
            // This should call executeMedialAxisGeneration(), NOT
            // executeGeneratePaths()
            bool success = pluginManager()->executeMedialAxisGeneration(selection, params);
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
        if (pluginManager()) {
          bool success = pluginManager()->executeMedialAxisGeneration(selection, params);
          if (!success) {
            // Error handling is done within executeMedialAxisGeneration
            return false;
          }
        }
        return true;
      },
      true);  // true = show errors to user
}

}  // namespace Commands
}  // namespace ChipCarving

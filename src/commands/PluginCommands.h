/**
 * Fusion 360 command handlers using dependency injection
 * Thin wrappers around core business logic
 */

#pragma once

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <map>
#include <memory>
#include <string>

#include "adapters/IFusionInterface.h"

namespace ChipCarving {
namespace Core {
class PluginManager;
}
}  // namespace ChipCarving

namespace ChipCarving {
namespace Commands {

/**
 * Base class for command handlers
 * Provides common functionality and dependency injection
 */
class BaseCommandHandler : public adsk::core::CommandCreatedEventHandler {
 public:
  explicit BaseCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager);
  virtual ~BaseCommandHandler() = default;

 protected:
  std::shared_ptr<Core::PluginManager> pluginManager_{};
};

/**
 * Import Design command handler
 */
class ImportDesignCommandHandler : public BaseCommandHandler {
 public:
  explicit ImportDesignCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager);

  void notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) override;

 private:
  void executeImportDesign(const adsk::core::Ptr<adsk::core::CommandEventArgs>& args);
  void handleInputChanged(const adsk::core::Ptr<adsk::core::InputChangedEventArgs>& args);

  std::string selectedFilePath_{};  // Store the selected file path
};

/**
 * Generate Paths command handler with enhanced UI
 */
class GeneratePathsCommandHandler : public BaseCommandHandler {
 public:
  explicit GeneratePathsCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager);

  void notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) override;

 private:
  // Helper methods for dialog creation
  void createParameterInputs(adsk::core::Ptr<adsk::core::CommandInputs> inputs);
  Adapters::MedialAxisParameters getParametersFromInputs(adsk::core::Ptr<adsk::core::CommandInputs> inputs);
  Adapters::SketchSelection getSelectionFromInputs(adsk::core::Ptr<adsk::core::CommandInputs> inputs);

  // Enhanced UI Phase 4: Command execution
  void executeMedialAxisProcessing(adsk::core::Ptr<adsk::core::CommandInputs> inputs);

  // Immediate geometry extraction (prevents stale tokens)
  void clearCachedGeometry();
  void extractAndCacheProfileGeometry(adsk::core::Ptr<adsk::fusion::Profile> profile, int index);

  // Selection validation
  bool isPartOfClosedProfile(adsk::core::Ptr<adsk::fusion::SketchCurve> curve);
  void validateAndCleanSelection(adsk::core::Ptr<adsk::core::SelectionCommandInput> selectionInput);

  // Sketch tracking for incremental generation
  std::map<std::string, std::string> toolToSketchMap_{};  // Maps tool name to sketch name

  // Cached geometry to avoid stale token issues
  std::vector<Adapters::ProfileGeometry> cachedProfiles_{};
};

}  // namespace Commands
}  // namespace ChipCarving

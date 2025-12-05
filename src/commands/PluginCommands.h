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
#include <vector>

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
  ~BaseCommandHandler() override = default;

  // Non-copyable, non-movable (prevent slicing, handlers are registered by pointer)
  BaseCommandHandler(const BaseCommandHandler&) = delete;
  BaseCommandHandler& operator=(const BaseCommandHandler&) = delete;
  BaseCommandHandler(BaseCommandHandler&&) = delete;
  BaseCommandHandler& operator=(BaseCommandHandler&&) = delete;

 protected:
  std::shared_ptr<Core::PluginManager> pluginManager() const {
    return pluginManager_;
  }

 private:
  std::shared_ptr<Core::PluginManager> pluginManager_;
};

/**
 * Import Design command handler
 */
class ImportDesignCommandHandler : public BaseCommandHandler {
 public:
  explicit ImportDesignCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager);
  ~ImportDesignCommandHandler() override;

  void notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) override;

 private:
  void executeImportDesign(const adsk::core::Ptr<adsk::core::CommandEventArgs>& args);
  void handleInputChanged(const adsk::core::Ptr<adsk::core::InputChangedEventArgs>& args);
  void cleanupEventHandlers();

  std::string selectedFilePath_;

  // Event handlers for cleanup (Issue #1: Event Handler Memory Management)
  std::vector<adsk::core::CommandEventHandler*> commandEventHandlers_;
  std::vector<adsk::core::InputChangedEventHandler*> inputChangedHandlers_;
};

/**
 * Generate Paths command handler with enhanced UI
 */
class GeneratePathsCommandHandler : public BaseCommandHandler {
 public:
  explicit GeneratePathsCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager);
  ~GeneratePathsCommandHandler() override;

  void notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) override;

 private:
  // Helper methods for dialog creation
  void createParameterInputs(const adsk::core::Ptr<adsk::core::CommandInputs>& inputs);
  Adapters::MedialAxisParameters getParametersFromInputs(const adsk::core::Ptr<adsk::core::CommandInputs>& inputs);
  Adapters::SketchSelection getSelectionFromInputs(const adsk::core::Ptr<adsk::core::CommandInputs>& inputs);

  // Enhanced UI Phase 4: Command execution
  void executeMedialAxisProcessing(const adsk::core::Ptr<adsk::core::CommandInputs>& inputs);

  // Immediate geometry extraction (prevents stale tokens)
  void clearCachedGeometry();
  void extractAndCacheProfileGeometry(const adsk::core::Ptr<adsk::fusion::Profile>& profile, int index);

  // Selection validation
  bool isPartOfClosedProfile(const adsk::core::Ptr<adsk::fusion::SketchCurve>& curve);
  void validateAndCleanSelection(const adsk::core::Ptr<adsk::core::SelectionCommandInput>& selectionInput);

  // Sketch tracking for incremental generation
  std::map<std::string, std::string> toolToSketchMap_;

  // Cached geometry to avoid stale token issues
  std::vector<Adapters::ProfileGeometry> cachedProfiles_;

  // Event handlers for cleanup (Issue #1: Event Handler Memory Management)
  std::vector<adsk::core::CommandEventHandler*> commandEventHandlers_;
  std::vector<adsk::core::InputChangedEventHandler*> inputChangedHandlers_;
  void cleanupEventHandlers();
};

}  // namespace Commands
}  // namespace ChipCarving

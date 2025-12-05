/**
 * SettingsCommand.h
 *
 * Settings command for controlling plugin preferences
 * Provides UI for configuring logging levels and other settings
 */

#pragma once

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <memory>
#include <vector>

namespace ChipCarving {

namespace Core {
class PluginManager;
}

namespace Commands {

/**
 * Handler for the Settings command
 * Opens a dialog to configure plugin settings including log visibility
 */
class SettingsCommandHandler : public adsk::core::CommandCreatedEventHandler {
 public:
  explicit SettingsCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager);
  ~SettingsCommandHandler() override;
  void notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) override;

  // Non-copyable, non-movable (prevent slicing, handlers are registered by pointer)
  SettingsCommandHandler(const SettingsCommandHandler&) = delete;
  SettingsCommandHandler& operator=(const SettingsCommandHandler&) = delete;
  SettingsCommandHandler(SettingsCommandHandler&&) = delete;
  SettingsCommandHandler& operator=(SettingsCommandHandler&&) = delete;

 private:
  std::shared_ptr<Core::PluginManager> pluginManager_;

  /**
   * Creates the settings dialog inputs
   */
  void createSettingsInputs(adsk::core::Ptr<adsk::core::CommandInputs> inputs);

  /**
   * Applies settings from the dialog inputs
   */
  void applySettings(adsk::core::Ptr<adsk::core::CommandInputs> inputs);

  /**
   * Cleans up event handlers to prevent memory leaks
   */
  void cleanupEventHandlers();

  // Event handlers for cleanup (Issue #1: Event Handler Memory Management)
  std::vector<adsk::core::CommandEventHandler*> commandEventHandlers_;
};

}  // namespace Commands
}  // namespace ChipCarving

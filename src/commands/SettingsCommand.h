/**
 * SettingsCommand.h
 *
 * Settings command for controlling plugin preferences
 * Provides UI for configuring logging levels and other settings
 */

#ifndef SRC_COMMANDS_SETTINGSCOMMAND_H_
#define SRC_COMMANDS_SETTINGSCOMMAND_H_

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <memory>

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
  void notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) override;

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
};

}  // namespace Commands
}  // namespace ChipCarving

#endif  // SRC_COMMANDS_SETTINGSCOMMAND_H_

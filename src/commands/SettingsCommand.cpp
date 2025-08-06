/**
 * SettingsCommand.cpp
 *
 * Implementation of the Settings command
 * Provides UI for configuring plugin settings including log visibility
 */

#include "SettingsCommand.h"

#include "../../include/utils/logging.h"
#include "../core/PluginManager.h"

namespace ChipCarving {
namespace Commands {

SettingsCommandHandler::SettingsCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager)
    : pluginManager_(pluginManager) {}

void SettingsCommandHandler::notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) {
  try {
    if (!eventArgs || !pluginManager_) {
      return;
    }

    adsk::core::Ptr<adsk::core::Command> cmd = eventArgs->command();
    if (!cmd) {
      return;
    }

    // Set command properties
    cmd->isOKButtonVisible(true);
    cmd->okButtonText("Apply");
    cmd->cancelButtonText("Cancel");
    cmd->isRepeatable(false);

    // Set dialog size
    cmd->setDialogInitialSize(400, 300);
    cmd->setDialogMinimumSize(350, 250);

    // Create command inputs
    adsk::core::Ptr<adsk::core::CommandInputs> inputs = cmd->commandInputs();
    if (!inputs) {
      return;
    }

    createSettingsInputs(inputs);

    // Create and register execute handler
    class ExecuteHandler : public adsk::core::CommandEventHandler {
     public:
      explicit ExecuteHandler(SettingsCommandHandler* parent) : parent_(parent) {}
      void notify(const adsk::core::Ptr<adsk::core::CommandEventArgs>& eventArgs) override {
        if (parent_ && eventArgs && eventArgs->command() && eventArgs->command()->commandInputs()) {
          parent_->applySettings(eventArgs->command()->commandInputs());
        }
      }

     private:
      SettingsCommandHandler* parent_;
    };

    auto onExecute = new ExecuteHandler(this);
    cmd->execute()->add(onExecute);

  } catch (const std::exception& e) {
    LOG_ERROR("Exception in SettingsCommandHandler::notify: " << e.what());
  } catch (...) {
    LOG_ERROR("Unknown exception in SettingsCommandHandler::notify");
  }
}

void SettingsCommandHandler::createSettingsInputs(adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
  try {
    // Add title
    adsk::core::Ptr<adsk::core::TextBoxCommandInput> titleDesc =
        inputs->addTextBoxCommandInput("titleDescription", "",
                                       "<b>Carving Plugin Settings</b><br/>"
                                       "Configure plugin preferences and behavior",
                                       2, true);

    // Logging Settings Group
    adsk::core::Ptr<adsk::core::GroupCommandInput> loggingGroup =
        inputs->addGroupCommandInput("loggingGroup", "Logging Settings");
    loggingGroup->isExpanded(true);
    loggingGroup->isEnabledCheckBoxDisplayed(false);
    adsk::core::Ptr<adsk::core::CommandInputs> loggingInputs = loggingGroup->children();

    // Get current log level to set checkbox state
    LogLevel currentLevel = GetMinLogLevel();
    bool showInfoDebug = (currentLevel == LogLevel::INFO || currentLevel == LogLevel::LOG_DEBUG);

    // Add checkbox for INFO/DEBUG messages
    adsk::core::Ptr<adsk::core::BoolValueCommandInput> showInfoCheckbox = loggingInputs->addBoolValueInput(
        "showInfoDebugMessages", "Show INFO and DEBUG messages", true, "", showInfoDebug);
    showInfoCheckbox->tooltip("When enabled, displays detailed INFO and DEBUG log messages in the "
                              "Text Commands window.\n"
                              "When disabled, only WARNING and ERROR messages are shown.\n"
                              "Default: disabled (for cleaner output)");

    // Add info text
    adsk::core::Ptr<adsk::core::TextBoxCommandInput> infoText =
        loggingInputs->addTextBoxCommandInput("loggingInfo", "",
                                              "Note: This setting applies immediately and persists for the "
                                              "current session only.",
                                              1, true);

  } catch (const std::exception& e) {
    LOG_ERROR("Exception creating settings inputs: " << e.what());
  } catch (...) {
    LOG_ERROR("Unknown exception creating settings inputs");
  }
}

void SettingsCommandHandler::applySettings(adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
  try {
    // Get the checkbox value
    adsk::core::Ptr<adsk::core::BoolValueCommandInput> showInfoCheckbox = inputs->itemById("showInfoDebugMessages");

    if (showInfoCheckbox) {
      if (showInfoCheckbox->value()) {
        // Show INFO and DEBUG messages
#ifdef DEBUG
        SetMinLogLevel(LogLevel::LOG_DEBUG);
        LOG_INFO("Log level set to DEBUG (showing all messages)");
#else
        SetMinLogLevel(LogLevel::INFO);
        LOG_INFO("Log level set to INFO (showing INFO, WARNING, and ERROR "
                 "messages)");
#endif
      } else {
        // Show only WARNING and ERROR messages
        SetMinLogLevel(LogLevel::WARNING);
        LOG_WARNING("Log level set to WARNING (INFO and DEBUG messages hidden)");
      }
    }

  } catch (const std::exception& e) {
    LOG_ERROR("Exception applying settings: " << e.what());
  } catch (...) {
    LOG_ERROR("Unknown exception applying settings");
  }
}

}  // namespace Commands
}  // namespace ChipCarving

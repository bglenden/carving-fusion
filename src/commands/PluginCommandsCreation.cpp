/**
 * PluginCommandsCreation.cpp
 *
 * Command creation and notification handling for PluginCommands
 * Split from PluginCommands.cpp for maintainability
 */

#include "PluginCommands.h"

namespace ChipCarving {
namespace Commands {

// GeneratePathsCommandHandler Implementation
GeneratePathsCommandHandler::GeneratePathsCommandHandler(
    std::shared_ptr<Core::PluginManager> pluginManager)
    : BaseCommandHandler(pluginManager) {}

void GeneratePathsCommandHandler::notify(
    const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) {
    try {
        if (!eventArgs || !pluginManager_) {
            return;
        }

        adsk::core::Ptr<adsk::core::Command> cmd = eventArgs->command();
        if (!cmd) {
            return;
        }

        // Set command properties
        cmd->isOKButtonVisible(true);  // Show OK button for execution
        cmd->okButtonText("Generate");  // Change OK to Generate
        cmd->cancelButtonText("Cancel");   // Standard cancel button
        cmd->isRepeatable(false);        // For now, close after generation

        // Set dialog size to fit input fields more compactly
        cmd->setDialogInitialSize(420, 650);  // width, height in pixels
        cmd->setDialogMinimumSize(400, 550);  // minimum width, height

        // Create command inputs
        adsk::core::Ptr<adsk::core::CommandInputs> inputs = cmd->commandInputs();
        if (!inputs) {
            return;
        }

        createParameterInputs(inputs);

        // Set the command to be modal so it stays open for selection
        cmd->isExecutedWhenPreEmpted(false);

        // Enhanced UI Phase 2.5: Add command event handlers
        // Create and register execute handler
        class ExecuteHandler : public adsk::core::CommandEventHandler {
         public:
            explicit ExecuteHandler(GeneratePathsCommandHandler* parent) : parent_(parent) {}
            void notify(const adsk::core::Ptr<adsk::core::CommandEventArgs>& eventArgs) override {
                if (parent_ && eventArgs && eventArgs->command() && eventArgs->command()->commandInputs()) {
                    parent_->executeMedialAxisProcessing(eventArgs->command()->commandInputs());
                }
            }

         private:
            GeneratePathsCommandHandler* parent_;
        };

        auto onExecute = new ExecuteHandler(this);
        cmd->execute()->add(onExecute);

        // Create and register preview handler (minimal implementation)
        class PreviewHandler : public adsk::core::CommandEventHandler {
         public:
            explicit PreviewHandler(GeneratePathsCommandHandler* parent) : parent_(parent) {
                (void)parent_;  // Suppress unused field warning - placeholder for preview functionality
            }
            void notify(const adsk::core::Ptr<adsk::core::CommandEventArgs>& eventArgs) override {
                (void)eventArgs;  // Suppress unused parameter warning - required by Fusion API
                // Minimal preview handler - just validate inputs without executing
            }

         private:
            GeneratePathsCommandHandler* parent_;  // TODO(developer): Use parent for preview functionality
        };

        auto onPreview = new PreviewHandler(this);
        cmd->executePreview()->add(onPreview);

        // Create and register input changed handler (minimal implementation)
        class InputChangedHandler : public adsk::core::InputChangedEventHandler {
         public:
            explicit InputChangedHandler(GeneratePathsCommandHandler* parent) : parent_(parent) {
                (void)parent_;  // Suppress unused field warning - placeholder for input validation
            }
            void notify(const adsk::core::Ptr<adsk::core::InputChangedEventArgs>& eventArgs) override {
                (void)eventArgs;  // Suppress unused parameter warning - required by Fusion API
                // Minimal input changed handler for UI updates
            }

         private:
            GeneratePathsCommandHandler* parent_;  // TODO(developer): Use parent for input validation
        };

        auto onInputChanged = new InputChangedHandler(this);
        cmd->inputChanged()->add(onInputChanged);

    } catch (const std::exception& e) {
        // Handle known exceptions
        if (pluginManager_) {
            // TODO(dev): Add error logging through plugin manager
        }
    } catch (...) {
        // Handle unknown exceptions
        if (pluginManager_) {
            // TODO(dev): Add error logging through plugin manager
        }
    }
}

}  // namespace Commands
}  // namespace ChipCarving

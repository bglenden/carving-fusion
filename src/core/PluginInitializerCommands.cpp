/**
 * PluginInitializerCommands.cpp
 *
 * Command creation functions for PluginInitializer
 * Split from PluginInitializer.cpp for maintainability
 */

#include <Core/UserInterface/CommandControl.h>
#include <Core/UserInterface/ToolbarControls.h>

#include "PluginInitializer.h"
#include "PluginInitializerGlobals.h"
#include "utils/logging.h"

using adsk::core::CommandControl;
using adsk::core::CommandDefinition;
using adsk::core::CommandDefinitions;
using adsk::core::Ptr;
using adsk::core::ToolbarControls;

using ChipCarving::Internal::commandControls;
using ChipCarving::Internal::commandDefinitions;
using ChipCarving::Internal::generateHandler;
using ChipCarving::Internal::importHandler;
using ChipCarving::Internal::panel;
using ChipCarving::Internal::pluginManager;
using ChipCarving::Internal::settingsHandler;
using ChipCarving::Internal::ui;

namespace ChipCarving {

void PluginInitializer::CreateImportDesignCommand() {
  std::string cmdId = "ChipCarvingImportDesignCpp";

  Ptr<CommandDefinitions> cmdDefs = ui->commandDefinitions();
  if (!cmdDefs) {
    return;
  }

  Ptr<CommandDefinition> cmdDef = cmdDefs->itemById(cmdId);

  if (!cmdDef) {
    std::string cmdName = "Import Design";
    std::string cmdTooltip = "Import chip carving design from JSON file";
    cmdDef = cmdDefs->addButtonDefinition(cmdId, cmdName, cmdTooltip, "./resources/import");
    if (cmdDef) {
      commandDefinitions.push_back(cmdDef);
    } else {
      return;
    }
  }

  // Create and connect event handler
  if (!importHandler && pluginManager) {
    try {
      // Convert unique_ptr to shared_ptr for command handler
      std::shared_ptr<Core::PluginManager> sharedManager(pluginManager.get(), [](Core::PluginManager*) {});
      importHandler = std::make_shared<Commands::ImportDesignCommandHandler>(sharedManager);
      cmdDef->commandCreated()->add(importHandler.get());
    } catch (const std::exception& e) {
      LOG_ERROR("Failed to create import command handler: " << e.what());
      return;
    }
  }

  if (panel) {
    Ptr<ToolbarControls> controls = panel->controls();
    if (controls) {
      Ptr<CommandControl> cmdControl = controls->itemById(cmdId);
      if (!cmdControl) {
        cmdControl = controls->addCommand(cmdDef);
        if (cmdControl) {
          commandControls.push_back(cmdControl);
        }
      }
    }
  }
}

void PluginInitializer::CreateGeneratePathsCommand() {
  std::string cmdId = "ChipCarvingGeneratePathsCpp";

  Ptr<CommandDefinitions> cmdDefs = ui->commandDefinitions();
  if (!cmdDefs) {
    return;
  }

  Ptr<CommandDefinition> cmdDef = cmdDefs->itemById(cmdId);

  if (!cmdDef) {
    std::string cmdName = "Generate Paths";
    std::string cmdTooltip = "Generate CNC toolpaths from imported design";
    cmdDef = cmdDefs->addButtonDefinition(cmdId, cmdName, cmdTooltip, "./resources/generate");
    if (cmdDef) {
      commandDefinitions.push_back(cmdDef);
    } else {
      return;
    }
  }

  // Create and connect event handler
  if (!generateHandler && pluginManager) {
    try {
      // Convert unique_ptr to shared_ptr for command handler
      std::shared_ptr<Core::PluginManager> sharedManager(pluginManager.get(), [](Core::PluginManager*) {});
      generateHandler = std::make_shared<Commands::GeneratePathsCommandHandler>(sharedManager);
      cmdDef->commandCreated()->add(generateHandler.get());
    } catch (const std::exception& e) {
      LOG_ERROR("Failed to create generate paths command handler: " << e.what());
      return;
    }
  }

  if (panel) {
    Ptr<ToolbarControls> controls = panel->controls();
    if (controls) {
      Ptr<CommandControl> cmdControl = controls->itemById(cmdId);
      if (!cmdControl) {
        cmdControl = controls->addCommand(cmdDef);
        if (cmdControl) {
          commandControls.push_back(cmdControl);
        }
      }
    }
  }
}

void PluginInitializer::CreateSettingsCommand() {
  std::string cmdId = "ChipCarvingSettingsCpp";

  Ptr<CommandDefinitions> cmdDefs = ui->commandDefinitions();
  if (!cmdDefs) {
    return;
  }

  Ptr<CommandDefinition> cmdDef = cmdDefs->itemById(cmdId);

  if (!cmdDef) {
    std::string cmdName = "Settings";
    std::string cmdTooltip = "Configure plugin settings and preferences";
    cmdDef = cmdDefs->addButtonDefinition(cmdId, cmdName, cmdTooltip, "./resources/settings");
    if (cmdDef) {
      commandDefinitions.push_back(cmdDef);
    } else {
      return;
    }
  }

  // Create and connect event handler
  if (!settingsHandler && pluginManager) {
    try {
      // Convert unique_ptr to shared_ptr for command handler
      std::shared_ptr<Core::PluginManager> sharedManager(pluginManager.get(), [](Core::PluginManager*) {});
      settingsHandler = std::make_shared<Commands::SettingsCommandHandler>(sharedManager);
      cmdDef->commandCreated()->add(settingsHandler.get());
    } catch (const std::exception& e) {
      LOG_ERROR("Failed to create settings command handler: " << e.what());
      return;
    }
  }

  if (panel) {
    Ptr<ToolbarControls> controls = panel->controls();
    if (controls) {
      Ptr<CommandControl> cmdControl = controls->itemById(cmdId);
      if (!cmdControl) {
        cmdControl = controls->addCommand(cmdDef);
        if (cmdControl) {
          commandControls.push_back(cmdControl);
        }
      }
    }
  }
}

}  // namespace ChipCarving

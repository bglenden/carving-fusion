/**
 * PluginInitializer.cpp
 *
 * Core plugin initialization and shutdown functions
 * Command creation functions are in PluginInitializerCommands.cpp
 */

#include "PluginInitializer.h"

#include <Core/Application/Document.h>
#include <Core/CoreAll.h>
#include <Core/UserInterface/CommandControl.h>
#include <Core/UserInterface/Toolbar.h>
#include <Core/UserInterface/ToolbarPanel.h>
#include <Core/UserInterface/ToolbarPanels.h>
#include <Core/UserInterface/ToolbarTab.h>
#include <Core/UserInterface/ToolbarTabs.h>
#include <Core/UserInterface/Toolbars.h>
#include <Core/UserInterface/Workspace.h>
#include <Fusion/FusionAll.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>

#include "PluginInitializerGlobals.h"
#include "adapters/FusionAPIAdapter.h"
#include "utils/logging.h"
#include "version.h"

using adsk::core::Application;
using adsk::core::CommandControl;
using adsk::core::CommandDefinition;
using adsk::core::CommandDefinitions;
using adsk::core::Document;
using adsk::core::Palettes;
using adsk::core::Ptr;
using adsk::core::TextCommandPalette;
using adsk::core::ToolbarPanel;
using adsk::core::ToolbarPanels;
using adsk::core::UserInterface;
using adsk::core::Workspace;
using adsk::core::Workspaces;

namespace ChipCarving {

// Define the global variables (declared extern in PluginInitializerGlobals.h)
namespace Internal {
Ptr<Application> app;
Ptr<UserInterface> ui;
std::unique_ptr<Core::PluginManager> pluginManager;
Ptr<ToolbarPanel> panel;
std::vector<Ptr<CommandDefinition>> commandDefinitions;
std::vector<Ptr<CommandControl>> commandControls;
std::shared_ptr<Commands::ImportDesignCommandHandler> importHandler;
std::shared_ptr<Commands::GeneratePathsCommandHandler> generateHandler;
std::shared_ptr<Commands::SettingsCommandHandler> settingsHandler;
}  // namespace Internal

using Internal::app;
using Internal::commandControls;
using Internal::commandDefinitions;
using Internal::generateHandler;
using Internal::importHandler;
using Internal::panel;
using Internal::pluginManager;
using Internal::settingsHandler;
using Internal::ui;

PluginMode PluginInitializer::GetModeFromEnv() {
  const char* mode = std::getenv("CHIP_CARVING_PLUGIN_MODE");
  if (!mode) {
    return PluginMode::STANDARD;
  }

  std::string modeStr(mode);
  if (modeStr == "DEBUG") {
    return PluginMode::DEBUG_MODE;
  }
  if (modeStr == "COMMANDS_ONLY") {
    return PluginMode::COMMANDS_ONLY;
  }
  if (modeStr == "UI_SIMPLE") {
    return PluginMode::UI_SIMPLE;
  }
  if (modeStr == "REFACTORED") {
    return PluginMode::REFACTORED;
  }
  return PluginMode::STANDARD;
}

void PluginInitializer::LogMessage(const std::string& message) {
  if (!ui)
    return;

  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] ";
  ss << "ChipCarvingCpp: " << message;

  Ptr<Palettes> palettes = ui->palettes();
  if (palettes) {
    Ptr<TextCommandPalette> textPalette = palettes->itemById("TextCommands");
    if (textPalette) {
      textPalette->writeText(ss.str());
    }
  }
}

bool PluginInitializer::CreateToolbarPanel() {
  if (!ui) {
    return false;
  }
  Ptr<Workspaces> workspaces = ui->workspaces();
  if (!workspaces) {
    return false;
  }
  Ptr<Workspace> designWorkspace = workspaces->itemById("FusionSolidEnvironment");
  if (!designWorkspace) {
    return false;
  }

  // Check for active document first
  Ptr<Document> activeDoc = app->activeDocument();
  Ptr<ToolbarPanels> panels;
  panels = designWorkspace->toolbarPanels();
  if (!panels) {
    // Try using an existing panel instead of creating a new one
    Ptr<ToolbarPanel> addInsPanel = nullptr;

    // Try to get the workspace's toolbar panels through a different method
    if (designWorkspace->activate()) {
      panels = designWorkspace->toolbarPanels();
      if (panels) {
        // Try to find ADD-INS panel
        addInsPanel = panels->itemById("SolidScriptsAddinsPanel");
        if (addInsPanel) {
          panel = addInsPanel;
        }
      }
    }

    if (!panel) {
      return false;
    }
  }

  // Create or get the Chip Carving panel (if not already created through tab)
  if (!panel) {
    std::string panelId = "ChipCarvingPanelCpp";
    panel = panels->itemById(panelId);

    if (!panel) {
      panel = panels->add(panelId, "Carving", "SelectPanel", false);
      if (!panel) {
        return false;
      }
    }
  }

  // Create commands
  CreateImportDesignCommand();
  CreateGeneratePathsCommand();
  CreateSettingsCommand();

  return true;
}

bool PluginInitializer::InitializePlugin(const char* /* context */, PluginMode mode) {
  app = Application::get();
  if (!app) {
    return false;
  }

  ui = app->userInterface();
  if (!ui) {
    return false;
  }

  LOG_WARNING("Starting Chip Carving Paths C++ Add-in v" << ADDIN_VERSION_STRING);

  // Create plugin manager based on mode
  switch (mode) {
    case PluginMode::DEBUG_MODE:
      // Add debug-specific initialization
      break;
    case PluginMode::COMMANDS_ONLY:
      // Minimal command setup
      break;
    case PluginMode::UI_SIMPLE:
      // Simple UI setup
      break;
    case PluginMode::REFACTORED:
      // Use refactored plugin manager
      break;
    case PluginMode::STANDARD:
    default:
      break;
  }

  try {
    // Initialize plugin manager with factory
    std::string logPath = "/tmp/chip_carving_cpp.log";
    auto factory = std::make_unique<Adapters::FusionAPIFactory>(app, ui, logPath);
    pluginManager = std::make_unique<Core::PluginManager>(std::move(factory));
    if (!pluginManager->initialize()) {
      return false;
    }

    // Try toolbar creation
    if (!CreateToolbarPanel()) {
      // Continue anyway - plugin can still function
    }
    return true;
  } catch (const std::exception& e) {
    if (ui) {
      ui->messageBox("Failed to start Chip Carving Paths add-in: " + std::string(e.what()), "Chip Carving Paths Error");
    }
    return false;
  } catch (...) {
    if (ui) {
      ui->messageBox("Failed to start Chip Carving Paths add-in: Unknown error", "Chip Carving Paths Error");
    }
    return false;
  }
}

bool PluginInitializer::ShutdownPlugin() {
  // Clean up UI elements - use try-catch for resilience
  // If one cleanup step fails, continue with others
  try {
    for (auto& control : commandControls) {
      if (control && control->isValid()) {
        control->deleteMe();
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Command control cleanup failed: " << e.what());
  } catch (...) {
    LOG_ERROR("Command control cleanup failed: Unknown error");
  }
  commandControls.clear();

  try {
    for (auto& cmdDef : commandDefinitions) {
      if (cmdDef && cmdDef->isValid()) {
        cmdDef->deleteMe();
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Command definition cleanup failed: " << e.what());
  } catch (...) {
    LOG_ERROR("Command definition cleanup failed: Unknown error");
  }
  commandDefinitions.clear();

  try {
    if (pluginManager) {
      pluginManager->shutdown();
      pluginManager.reset();
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Plugin manager cleanup failed: " << e.what());
  } catch (...) {
    LOG_ERROR("Plugin manager cleanup failed: Unknown error");
  }

  return true;
}

}  // namespace ChipCarving

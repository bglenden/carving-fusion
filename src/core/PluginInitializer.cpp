#include "PluginInitializer.h"

#include "../adapters/FusionAPIAdapter.h"
#include "../commands/PluginCommands.h"
#include "../core/PluginManager.h"
#include "../version.h"
#include "../../include/utils/TempFileManager.h"
#include <cstdio>
// TODO(dev): Fix HelloWorldCommand API issues
// #include "../../include/commands/HelloWorldCommand.h"
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
#include <cstdlib>
#include <iomanip>
#include <sstream>

using namespace adsk::core;
using namespace adsk::fusion;

namespace ChipCarving {

// Global variables
static Ptr<Application> app;
static Ptr<UserInterface> ui;
static std::unique_ptr<Core::PluginManager> pluginManager;
static Ptr<ToolbarPanel> panel;
static std::vector<Ptr<CommandDefinition>> commandDefinitions;
static std::vector<Ptr<CommandControl>> commandControls;
static std::shared_ptr<Commands::ImportDesignCommandHandler> importHandler;
static std::shared_ptr<Commands::GeneratePathsCommandHandler> generateHandler;

PluginMode PluginInitializer::GetModeFromEnv() {
    const char* mode = std::getenv("CHIP_CARVING_PLUGIN_MODE");
    if (!mode) {
        return PluginMode::STANDARD;
    }

    std::string modeStr(mode);
    if (modeStr == "DEBUG") {
        return PluginMode::DEBUG_MODE;
    } else if (modeStr == "COMMANDS_ONLY") {
        return PluginMode::COMMANDS_ONLY;
    } else if (modeStr == "UI_SIMPLE") {
        return PluginMode::UI_SIMPLE;
    } else if (modeStr == "REFACTORED") {
        return PluginMode::REFACTORED;
    }

    return PluginMode::STANDARD;
}

void PluginInitializer::LogMessage(const std::string& message) {
    try {
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
    } catch (...) {
        // Ignore logging errors
    }
}

bool PluginInitializer::CreateToolbarPanel() {
    try {
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
        try {
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
        } catch (const std::exception& e) {
            return false;
        } catch (...) {
            return false;
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

        return true;
    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}


bool PluginInitializer::InitializePlugin(const char* /* context */, PluginMode mode) {
    try {
        app = Application::get();
        if (!app) {
            return false;
        }

        ui = app->userInterface();
        if (!ui) {
            return false;
        }

        LogMessage("Starting Chip Carving Paths C++ Add-in v" + std::string(ADDIN_VERSION_STRING));

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
            ui->messageBox("Failed to start Chip Carving Paths add-in: " + std::string(e.what()),
                           "Chip Carving Paths Error");
        }
        return false;
    } catch (...) {
        if (ui) {
            ui->messageBox("Failed to start Chip Carving Paths add-in: Unknown error",
                           "Chip Carving Paths Error");
        }
        return false;
    }
}

bool PluginInitializer::ShutdownPlugin() {
    try {
        // Clean up UI elements
        for (auto& control : commandControls) {
            if (control && control->isValid()) {
                control->deleteMe();
            }
        }
        commandControls.clear();

        for (auto& cmdDef : commandDefinitions) {
            if (cmdDef && cmdDef->isValid()) {
                cmdDef->deleteMe();
            }
        }
        commandDefinitions.clear();

        if (pluginManager) {
            pluginManager->shutdown();
            pluginManager.reset();
        }

        return true;
    } catch (...) {
        return false;
    }
}

void PluginInitializer::CreateImportDesignCommand() {
    try {
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
            // Convert unique_ptr to shared_ptr for command handler
            std::shared_ptr<Core::PluginManager> sharedManager(pluginManager.get(),
                                                               [](Core::PluginManager*) {});
            importHandler = std::make_shared<Commands::ImportDesignCommandHandler>(sharedManager);
            cmdDef->commandCreated()->add(importHandler.get());
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
    } catch (std::exception& e) {
        // Ignore errors
    }
}

void PluginInitializer::CreateGeneratePathsCommand() {
    try {
        std::string cmdId = "ChipCarvingGeneratePathsCpp";

        Ptr<CommandDefinitions> cmdDefs = ui->commandDefinitions();
        if (!cmdDefs) {
            return;
        }

        Ptr<CommandDefinition> cmdDef = cmdDefs->itemById(cmdId);

        if (!cmdDef) {
            std::string cmdName = "Generate Paths";
            std::string cmdTooltip = "Generate CNC toolpaths from imported design";
            cmdDef =
                cmdDefs->addButtonDefinition(cmdId, cmdName, cmdTooltip, "./resources/generate");
            if (cmdDef) {
                commandDefinitions.push_back(cmdDef);
            } else {
                return;
            }
        }

        // Create and connect event handler
        if (!generateHandler && pluginManager) {
            // Convert unique_ptr to shared_ptr for command handler
            std::shared_ptr<Core::PluginManager> sharedManager(pluginManager.get(),
                                                               [](Core::PluginManager*) {});
            generateHandler =
                std::make_shared<Commands::GeneratePathsCommandHandler>(sharedManager);
            cmdDef->commandCreated()->add(generateHandler.get());
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
    } catch (std::exception& e) {
        // Ignore errors
    }
}


}  // namespace ChipCarving

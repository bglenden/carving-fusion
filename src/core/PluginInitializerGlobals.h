/**
 * PluginInitializerGlobals.h
 *
 * Internal header for sharing global state between PluginInitializer*.cpp files
 * This is an implementation detail and should not be included outside of
 * PluginInitializer implementation files.
 */

#pragma once

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <memory>
#include <vector>

#include "PluginManager.h"
#include "commands/PluginCommands.h"
#include "commands/SettingsCommand.h"

namespace ChipCarving {
namespace Internal {

// Global state shared between PluginInitializer implementation files
// These are defined in PluginInitializer.cpp

extern adsk::core::Ptr<adsk::core::Application> app;
extern adsk::core::Ptr<adsk::core::UserInterface> ui;
extern std::unique_ptr<Core::PluginManager> pluginManager;
extern adsk::core::Ptr<adsk::core::ToolbarPanel> panel;
extern std::vector<adsk::core::Ptr<adsk::core::CommandDefinition>> commandDefinitions;
extern std::vector<adsk::core::Ptr<adsk::core::CommandControl>> commandControls;
extern std::shared_ptr<Commands::ImportDesignCommandHandler> importHandler;
extern std::shared_ptr<Commands::GeneratePathsCommandHandler> generateHandler;
extern std::shared_ptr<Commands::SettingsCommandHandler> settingsHandler;

}  // namespace Internal
}  // namespace ChipCarving

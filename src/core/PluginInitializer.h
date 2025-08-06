#pragma once

#include <memory>
#include <string>

namespace ChipCarving {

enum class PluginMode { STANDARD, DEBUG_MODE, COMMANDS_ONLY, UI_SIMPLE, REFACTORED };

class PluginInitializer {
 public:
  static PluginMode GetModeFromEnv();
  static bool InitializePlugin(const char* context, PluginMode mode);
  static bool ShutdownPlugin();

 private:
  static void LogMessage(const std::string& message);
  static bool CreateToolbarPanel();
  static void CreateImportDesignCommand();
  static void CreateGeneratePathsCommand();
  static void CreateSettingsCommand();
};

}  // namespace ChipCarving

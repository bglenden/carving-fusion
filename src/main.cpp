/**
 * Chip Carving Paths - C++ Fusion 360 Add-in
 * Consolidated entry point for all plugin modes
 *
 * Mode can be controlled via environment variable:
 * CHIP_CARVING_PLUGIN_MODE = STANDARD | DEBUG | COMMANDS_ONLY | UI_SIMPLE | REFACTORED
 */

#include "core/PluginInitializer.h"
#include "../include/utils/TempFileManager.h"
#include <cstdio>

extern "C" bool XPluginStart(const char* context) {
    // Debug: Write to file immediately to verify plugin is being called
    chip_carving::TempFileManager::ensureTempDirectoryExists();
    std::string logPath = chip_carving::TempFileManager::getLogFilePath("fusion_plugin_start.log");
    FILE* debugFile = fopen(logPath.c_str(), "a");
    if (debugFile) {
        fprintf(debugFile, "XPluginStart called at %s\n", __TIME__);
        fclose(debugFile);
    }

    // Get mode from environment variable or use default
    ChipCarving::PluginMode mode = ChipCarving::PluginInitializer::GetModeFromEnv();

    // Initialize plugin with selected mode
    return ChipCarving::PluginInitializer::InitializePlugin(context, mode);
}

extern "C" bool XPluginStop(const char* context) {
    (void)context;  // Suppress unused parameter warning
    // Shutdown plugin
    return ChipCarving::PluginInitializer::ShutdownPlugin();
}

// Also provide legacy names for compatibility
extern "C" bool run(const char* context) {
    return XPluginStart(context);
}

extern "C" bool stop(const char* context) {
    return XPluginStop(context);
}

#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#endif  // XI_WIN

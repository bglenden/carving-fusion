#include "../../include/utils/TempFileManager.h"
#include <cstdlib>
#include <filesystem>

namespace chip_carving {

const std::string TempFileManager::TEMP_DIR_NAME = "temp_output";

std::string TempFileManager::getTempDirectory() {
    // Get the plugin directory path
    // This will be relative to where the plugin is installed
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        homeDir = "/tmp";  // Fallback
    }

    // Create path in the plugin's source directory for development
    // In production, this could be changed to a user-specific location
    std::filesystem::path pluginPath = __FILE__;
    pluginPath = pluginPath.parent_path().parent_path().parent_path();  // Go up to fusion_plugin_cpp/

    std::filesystem::path tempPath = pluginPath / TEMP_DIR_NAME;
    return tempPath.string();
}

std::string TempFileManager::getLogFilePath(const std::string& filename) {
    std::filesystem::path tempDir = getTempDirectory();
    std::filesystem::path logDir = tempDir / "logs";

    // Ensure logs subdirectory exists
    std::filesystem::create_directories(logDir);

    return (logDir / filename).string();
}

std::string TempFileManager::getSVGFilePath(const std::string& filename) {
    std::filesystem::path tempDir = getTempDirectory();
    std::filesystem::path svgDir = tempDir / "svg";

    // Ensure svg subdirectory exists
    std::filesystem::create_directories(svgDir);

    return (svgDir / filename).string();
}

void TempFileManager::ensureTempDirectoryExists() {
    std::filesystem::path tempDir = getTempDirectory();
    std::filesystem::create_directories(tempDir);

    // Also create subdirectories
    std::filesystem::create_directories(tempDir / "logs");
    std::filesystem::create_directories(tempDir / "svg");
}

}  // namespace chip_carving

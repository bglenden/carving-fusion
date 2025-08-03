#pragma once

#include <string>
#include <filesystem>

namespace chip_carving {

class TempFileManager {
public:
    // Get the base temp directory path
    static std::string getTempDirectory();
    
    // Get path for a log file
    static std::string getLogFilePath(const std::string& filename);
    
    // Get path for an SVG file
    static std::string getSVGFilePath(const std::string& filename);
    
    // Ensure temp directory exists
    static void ensureTempDirectoryExists();
    
private:
    static const std::string TEMP_DIR_NAME;
};

} // namespace chip_carving
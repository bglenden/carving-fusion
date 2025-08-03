#include "../../include/utils/logging.h"

#include <Core/CoreAll.h>

#include <chrono>
#include <iomanip>
#include <sstream>

using namespace adsk::core;

void LogToConsole(const std::string& message) {
    try {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        // Format timestamp
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

        // Create log message
        std::string logMessage = "[" + ss.str() + "] " + message;

        // Get the application and UI
        Ptr<Application> app = Application::get();
        if (!app)
            return;

        Ptr<UserInterface> ui = app->userInterface();
        if (!ui)
            return;

        // Try to write to Text Commands palette
        Ptr<Palette> textPalette = ui->palettes()->itemById("TextCommands");
        if (textPalette) {
            Ptr<TextCommandPalette> textCommandPalette = textPalette;
            textCommandPalette->writeText(logMessage);
        }
    } catch (...) {
        // Silently fail if logging doesn't work
    }
}

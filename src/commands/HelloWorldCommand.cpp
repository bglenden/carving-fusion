#include "../../include/commands/HelloWorldCommand.h"

#include "../../include/utils/logging.h"

using namespace adsk::core;
using namespace adsk::fusion;

// Static handlers to prevent garbage collection
static HelloWorldCommandCreatedHandler* cmdCreatedHandler = nullptr;
static HelloWorldCommandExecuteHandler* cmdExecuteHandler = nullptr;

void CreateHelloWorldCommand(
    Ptr<Application> app, Ptr<UserInterface> ui, Ptr<ToolbarPanel> panel,
    std::vector<Ptr<CommandDefinition>>& commandDefinitions,
    std::vector<Ptr<Control>>& controls) {
  try {
    // Command ID and info
    std::string cmdId = "ChipCarvingHelloWorldCpp";

    LogToConsole("Creating Hello World command: " + cmdId);

    // Check if command already exists
    Ptr<CommandDefinition> cmdDef = ui->commandDefinitions()->itemById(cmdId);

    if (!cmdDef) {
      // Create the command definition
      std::string cmdName = "Hello World";
      std::string cmdTooltip = "Test C++ add-in functionality";
      cmdDef = ui->commandDefinitions()->addButtonDefinition(
          cmdId, cmdName, cmdTooltip,
          "./resources/hello");  // Icon folder

      LogToConsole("Hello World command definition created");

      // Store the command definition
      commandDefinitions.push_back(cmdDef);
    } else {
      LogToConsole("Hello World command definition already exists");
    }

    // Create the command created event handler
    if (!cmdCreatedHandler) {
      cmdCreatedHandler = new HelloWorldCommandCreatedHandler();
    n}
    cmdDef->commandCreated()->add(cmdCreatedHandler);
    LogToConsole("Command created handler connected");

    // Add command to panel
    Ptr<CommandControl> cmdControl = panel->controls()->itemById(cmdId);
    if (!cmdControl) {
      cmdControl = panel->controls()->addCommand(cmdDef);
      controls.push_back(cmdControl);
      LogToConsole("Hello World command added to panel");
    } else {
      LogToConsole("Hello World command already in panel");
    }
  } catch (std::exception& e) {
    LogToConsole("ERROR creating Hello World command: " +
                 std::string(e.what()));
    throw;
  }
}

void HelloWorldCommandCreatedHandler::notify(
    const Ptr<CommandCreatedEventArgs>& eventArgs) {
  try {
    LogToConsole("Hello World command created event fired");

    if (!eventArgs) return;

    Ptr<Command> cmd = eventArgs->command();
    if (!cmd) return;

    // Set the command to execute when created (no dialog)
    cmd->isOKButtonVisible(false);

    // Create and connect the execute handler
    if (!cmdExecuteHandler) {
      cmdExecuteHandler = new HelloWorldCommandExecuteHandler();
    }
    cmd->execute()->add(cmdExecuteHandler);

    // Execute immediately
    eventArgs->isValidResult(true);
  } catch (std::exception& e) {
    LogToConsole("ERROR in command created handler: " + std::string(e.what()));
  }
}

void HelloWorldCommandExecuteHandler::notify(
    const Ptr<CommandEventArgs>& eventArgs) {
  try {
    LogToConsole("Hello World command execute event fired");

    // Get the application and UI
    Ptr<Application> app = Application::get();
    if (!app) return;

    Ptr<UserInterface> ui = app->userInterface();
    if (!ui) return;

    // Show message box
    std::string message = "Hello from C++ Add-in!\n\n";
    message += "This confirms that:\n";
    message += "\u2713 C++ add-in is working\n";
    message += "\u2713 Commands can be created\n";
    message += "\u2713 Event handlers are functional\n\n";
    message += "Next step: Add OpenVoronoi integration";

    ui->messageBox(message, "Chip Carving C++ Add-in");

    LogToConsole("Hello World message displayed successfully");
  } catch (std::exception& e) {
    LogToConsole("ERROR in command execute handler: " + std::string(e.what()));
  }
}
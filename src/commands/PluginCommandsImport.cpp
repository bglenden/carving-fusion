/**
 * PluginCommandsImport.cpp
 *
 * Import Design command handlers
 * Split from PluginCommands.cpp for maintainability
 */

#include <iostream>

#include "core/PluginManager.h"
#include "utils/UnitConversion.h"
#include "PluginCommands.h"

namespace ChipCarving {
namespace Commands {

// BaseCommandHandler Implementation
BaseCommandHandler::BaseCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager)
    : pluginManager_(pluginManager) {}

// ImportDesignCommandHandler Implementation
ImportDesignCommandHandler::ImportDesignCommandHandler(std::shared_ptr<Core::PluginManager> pluginManager)
    : BaseCommandHandler(pluginManager) {}

void ImportDesignCommandHandler::notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) {
  try {
    if (!eventArgs)
      return;

    auto command = eventArgs->command();
    if (!command)
      return;

    // Set dialog size to fit input fields more compactly
    command->setDialogInitialSize(400, 350);  // width, height in pixels
    command->setDialogMinimumSize(380, 300);  // minimum width, height

    // Create command inputs for the dialog
    auto inputs = command->commandInputs();
    if (!inputs)
      return;

    // Add wide title to make dialog wider
    inputs->addTextBoxCommandInput("titleText", "",
                                   "<b>Import Design</b><br/>Import a JSON chip carving design file with "
                                   "Leaf and TriArc shapes, then optionally select a construction plane "
                                   "or surface for placement.",
                                   3, true);

    // Add file selection input
    inputs->addBoolValueInput("fileSelectionButton", "Select Design File", false, "", true);

    // Add text input to show selected file (read-only)
    auto filePathInput = inputs->addStringValueInput("selectedFilePath", "Selected File", "No file selected");
    filePathInput->isReadOnly(true);

    // Add plane/surface selection input
    auto planeSelection = inputs->addSelectionInput("targetPlane", "Target Plane/Surface (Optional)",
                                                    "Select construction plane or flat surface");
    planeSelection->addSelectionFilter("ConstructionPlanes");
    planeSelection->addSelectionFilter("PlanarFaces");
    planeSelection->tooltip("Optional: Select a construction plane or flat surface for the sketch. "
                            "Must be parallel to XY plane. Defaults to XY plane if not selected.");

    // Create and register execute handler
    class ExecuteHandler : public adsk::core::CommandEventHandler {
     public:
      explicit ExecuteHandler(ImportDesignCommandHandler* parent) : parent_(parent) {}
      void notify(const adsk::core::Ptr<adsk::core::CommandEventArgs>& eventArgs) override {
        parent_->executeImportDesign(eventArgs);
      }

     private:
      ImportDesignCommandHandler* parent_;
    };

    auto onExecute = new ExecuteHandler(this);
    command->execute()->add(onExecute);

    // Create and register input changed handler
    class InputChangedHandler : public adsk::core::InputChangedEventHandler {
     public:
      explicit InputChangedHandler(ImportDesignCommandHandler* parent) : parent_(parent) {}
      void notify(const adsk::core::Ptr<adsk::core::InputChangedEventArgs>& eventArgs) override {
        parent_->handleInputChanged(eventArgs);
      }

     private:
      ImportDesignCommandHandler* parent_;
    };

    auto onInputChanged = new InputChangedHandler(this);
    command->inputChanged()->add(onInputChanged);
  } catch (const std::exception& e) {
    std::cerr << "Import design command setup error: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown error in import design command setup" << std::endl;
  }
}

void ImportDesignCommandHandler::handleInputChanged(const adsk::core::Ptr<adsk::core::InputChangedEventArgs>& args) {
  if (!args)
    return;

  auto inputs = args->inputs();
  if (!inputs)
    return;

  auto changedInput = args->input();
  if (!changedInput)
    return;

  // Check if file selection button was clicked
  if (changedInput->id() == "fileSelectionButton") {
    // Show file dialog
    if (pluginManager_) {
      auto factory = pluginManager_->getFactory();
      if (factory) {
        auto ui = factory->createUserInterface();
        if (ui) {
          selectedFilePath_ = ui->showFileDialog("Select Design File", "JSON Files (*.json)");

          // Update the file path display
          auto filePathInput = inputs->itemById("selectedFilePath");
          if (filePathInput) {
            auto stringInput = filePathInput->cast<adsk::core::StringValueCommandInput>();
            if (stringInput) {
              if (!selectedFilePath_.empty()) {
                // Show just the filename, not the full path
                size_t lastSlash = selectedFilePath_.find_last_of("/\\");
                std::string filename =
                    (lastSlash != std::string::npos) ? selectedFilePath_.substr(lastSlash + 1) : selectedFilePath_;
                stringInput->value(filename);
              } else {
                stringInput->value("No file selected");
              }
            }
          }
        }
      }
    }
  }
}

void ImportDesignCommandHandler::executeImportDesign(const adsk::core::Ptr<adsk::core::CommandEventArgs>& args) {
  if (!args || !pluginManager_)
    return;

  try {
    auto inputs = args->command()->commandInputs();
    if (!inputs)
      return;

    // Check if a file was selected
    if (selectedFilePath_.empty()) {
      if (pluginManager_) {
        auto factory = pluginManager_->getFactory();
        if (factory) {
          auto ui = factory->createUserInterface();
          if (ui) {
            ui->showMessageBox("Import Design", "Please select a JSON design file.");
          }
        }
      }
      return;
    }

    // Get the selected plane/surface if any
    std::string planeEntityId;
    auto planeSelection = inputs->itemById("targetPlane");
    if (planeSelection) {
      auto selectionInput = planeSelection->cast<adsk::core::SelectionCommandInput>();
      if (selectionInput && selectionInput->selectionCount() > 0) {
        auto selection = selectionInput->selection(0);
        if (selection) {
          auto entity = selection->entity();
          if (entity) {
            // Try to cast to construction plane
            auto constructionPlane = entity->cast<adsk::fusion::ConstructionPlane>();
            if (constructionPlane) {
              planeEntityId = constructionPlane->entityToken();
            } else {
              // Try to cast to BRepFace
              auto face = entity->cast<adsk::fusion::BRepFace>();
              if (face) {
                planeEntityId = face->entityToken();
              }
            }
          }
        }
      }
    }

    // Execute the import with the selected file and optional plane
    pluginManager_->executeImportDesign(selectedFilePath_, planeEntityId);
  } catch (const std::exception& e) {
    std::cerr << "Error executing import design: " << e.what() << std::endl;
  }
}

}  // namespace Commands
}  // namespace ChipCarving

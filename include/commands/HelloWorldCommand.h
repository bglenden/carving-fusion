#ifndef HELLO_WORLD_COMMAND_H
#define HELLO_WORLD_COMMAND_H

#include <Core/CoreAll.h>

#include <vector>

/**
 * Create the Hello World command
 */
void CreateHelloWorldCommand(
    adsk::core::Ptr<adsk::core::Application> app, adsk::core::Ptr<adsk::core::UserInterface> ui,
    adsk::core::Ptr<adsk::core::ToolbarPanel> panel,
    std::vector<adsk::core::Ptr<adsk::core::CommandDefinition>>& commandDefinitions,
    std::vector<adsk::core::Ptr<adsk::core::Control>>& controls);

/**
 * Command created event handler for Hello World
 */
class HelloWorldCommandCreatedHandler : public adsk::core::CommandCreatedEventHandler {
   public:
    void notify(const adsk::core::Ptr<adsk::core::CommandCreatedEventArgs>& eventArgs) override;
};

/**
 * Command execute event handler for Hello World
 */
class HelloWorldCommandExecuteHandler : public adsk::core::CommandEventHandler {
   public:
    void notify(const adsk::core::Ptr<adsk::core::CommandEventArgs>& eventArgs) override;
};

#endif  // HELLO_WORLD_COMMAND_H
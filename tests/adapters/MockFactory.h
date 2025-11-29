/**
 * MockFactory.h
 * Mock factory for creating test objects - provides dependency injection for testing
 */

#pragma once

#include <memory>

#include "MockLogger.h"
#include "MockUserInterface.h"
#include "MockWorkspace.h"
#include "adapters/IFusionInterface.h"

using namespace ChipCarving::Adapters;

/**
 * Non-deleting deleter for shared mock objects
 */
template <typename T>
struct NoOpDeleter {
  void operator()(T*) const {
    // Do nothing - shared_ptr manages the lifetime
  }
};

class MockFactory : public IFusionFactory {
 public:
  MockFactory() {
    mockLogger_ = std::make_shared<MockLogger>();
    mockUI_ = std::make_shared<MockUserInterface>();
    mockWorkspace_ = std::make_shared<MockWorkspace>();
  }

  std::unique_ptr<ILogger> createLogger() override {
    auto logger = std::make_unique<MockLogger>();
    lastCreatedLogger_ = logger.get();
    return logger;
  }

  std::unique_ptr<IUserInterface> createUserInterface() override {
    auto ui = std::make_unique<MockUserInterface>();
    lastCreatedUI_ = ui.get();
    return ui;
  }

  std::unique_ptr<IWorkspace> createWorkspace() override {
    auto workspace = std::make_unique<MockWorkspace>();
    lastCreatedWorkspace_ = workspace.get();
    return workspace;
  }

  // Access to mock objects for test verification
  MockLogger* getLastCreatedLogger() { return lastCreatedLogger_; }
  MockUserInterface* getLastCreatedUI() { return lastCreatedUI_; }
  MockWorkspace* getLastCreatedWorkspace() { return lastCreatedWorkspace_; }

  // Legacy accessors for compatibility
  std::shared_ptr<MockLogger> getMockLogger() { return mockLogger_; }
  std::shared_ptr<MockUserInterface> getMockUI() { return mockUI_; }
  std::shared_ptr<MockWorkspace> getMockWorkspace() { return mockWorkspace_; }

 private:
  std::shared_ptr<MockLogger> mockLogger_;
  std::shared_ptr<MockUserInterface> mockUI_;
  std::shared_ptr<MockWorkspace> mockWorkspace_;

  MockLogger* lastCreatedLogger_ = nullptr;
  MockUserInterface* lastCreatedUI_ = nullptr;
  MockWorkspace* lastCreatedWorkspace_ = nullptr;
};

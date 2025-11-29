/**
 * MockUserInterface.h
 * Mock user interface for testing - captures UI interactions for verification
 */

#pragma once

#include <string>

#include "adapters/IFusionInterface.h"

using namespace ChipCarving::Adapters;

class MockUserInterface : public IUserInterface {
 public:
  void showMessageBox(const std::string& title, const std::string& message) override {
    lastMessageBoxTitle = title;
    lastMessageBoxMessage = message;
    messageBoxCallCount++;
  }

  std::string showFileDialog(const std::string& title, const std::string& filter) override {
    lastFileDialogTitle = title;
    lastFileDialogFilter = filter;
    return mockFileDialogPath;
  }

  std::string selectJsonFile() override { return mockJsonFilePath; }

  bool confirmAction(const std::string& message) override {
    lastConfirmMessage = message;
    return mockConfirmResult;
  }

  bool showParameterDialog(const std::string& title, MedialAxisParameters& params) override {
    lastParameterDialogTitle = title;
    lastParameterDialogParams = params;
    parameterDialogCallCount++;

    if (mockParameterDialogResult) {
      params = mockParameterValues;
    }

    return mockParameterDialogResult;
  }

  SketchSelection showSketchSelectionDialog(const std::string& title) override {
    lastSketchSelectionDialogTitle = title;
    sketchSelectionDialogCallCount++;
    return mockSketchSelection;
  }

  void updateSelectionCount(int count) override {
    lastSelectionCount = count;
    updateSelectionCountCallCount++;
  }

  // Test helpers
  std::string lastMessageBoxTitle;
  std::string lastMessageBoxMessage;
  int messageBoxCallCount = 0;

  std::string lastFileDialogTitle;
  std::string lastFileDialogFilter;
  std::string mockFileDialogPath = "/test/path/design.json";

  std::string lastConfirmMessage;
  bool mockConfirmResult = true;

  std::string mockJsonFilePath = "/test/path/design.json";

  std::string lastParameterDialogTitle;
  MedialAxisParameters lastParameterDialogParams{};
  int parameterDialogCallCount = 0;
  bool mockParameterDialogResult = true;
  MedialAxisParameters mockParameterValues{};

  std::string lastSketchSelectionDialogTitle;
  int sketchSelectionDialogCallCount = 0;
  SketchSelection mockSketchSelection{};

  int lastSelectionCount = 0;
  int updateSelectionCountCallCount = 0;

  void reset() {
    lastMessageBoxTitle.clear();
    lastMessageBoxMessage.clear();
    messageBoxCallCount = 0;
    lastFileDialogTitle.clear();
    lastFileDialogFilter.clear();
    mockFileDialogPath = "/test/path/design.json";
    lastConfirmMessage.clear();
    mockConfirmResult = true;
    mockJsonFilePath = "/test/path/design.json";

    lastParameterDialogTitle.clear();
    parameterDialogCallCount = 0;
    mockParameterDialogResult = true;
    mockParameterValues = MedialAxisParameters{};
    lastSketchSelectionDialogTitle.clear();
    sketchSelectionDialogCallCount = 0;
    mockSketchSelection = SketchSelection{};
    lastSelectionCount = 0;
    updateSelectionCountCallCount = 0;
  }
};

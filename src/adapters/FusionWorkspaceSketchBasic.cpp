/**
 * FusionWorkspaceSketchBasic.cpp
 *
 * Basic sketch creation operations for FusionWorkspace
 * Split from FusionWorkspaceSketch.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#include "FusionAPIAdapter.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

// Constructor - anchors the vtable in this compilation unit
FusionWorkspace::FusionWorkspace(Ptr<Application> app) : app_(app) {}

std::unique_ptr<ISketch> FusionWorkspace::createSketch(const std::string& name) {
  if (!app_) {
    return nullptr;
  }

  try {
    // Get the active design
    Ptr<adsk::fusion::Design> design = app_->activeProduct();
    if (!design) {
      return nullptr;
    }

    // Get the root component
    Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
    if (!rootComp) {
      return nullptr;
    }

    // Get the XY plane for the sketch
    Ptr<adsk::fusion::ConstructionPlane> xyPlane = rootComp->xYConstructionPlane();
    if (!xyPlane) {
      return nullptr;
    }

    // Create the sketch
    Ptr<adsk::fusion::Sketches> sketches = rootComp->sketches();
    if (!sketches) {
      return nullptr;
    }

    Ptr<adsk::fusion::Sketch> sketch = sketches->add(xyPlane);
    if (!sketch) {
      return nullptr;
    }

    // Set the sketch name
    sketch->name(name);

    return std::make_unique<FusionSketch>(name, app_, sketch);
  } catch (const std::exception& e) {
    std::cout << "Sketch creation error: " << e.what() << std::endl;
    return nullptr;
  } catch (...) {
    std::cout << "Unknown sketch creation error" << std::endl;
    return nullptr;
  }
}

}  // namespace Adapters
}  // namespace ChipCarving

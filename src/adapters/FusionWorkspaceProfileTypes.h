/**
 * FusionWorkspaceProfileTypes.h
 *
 * Shared type definitions for profile processing operations
 * Split from FusionWorkspaceProfile.cpp for maintainability
 */

#pragma once

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <vector>

namespace ChipCarving {
namespace Adapters {

// Structure to hold curve data for chaining
struct CurveData {
  size_t originalIndex;
  std::vector<adsk::core::Ptr<adsk::core::Point3D>> strokePoints;
  adsk::core::Ptr<adsk::core::Point3D> startPoint;
  adsk::core::Ptr<adsk::core::Point3D> endPoint;
  bool used;
};

}  // namespace Adapters
}  // namespace ChipCarving

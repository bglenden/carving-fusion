/**
 * PluginCommandsGeometryChaining.h
 *
 * Header for geometry curve chaining implementation
 * Part of PluginCommandsGeometry refactoring (Item #1a)
 */

#pragma once

#include <cmath>
#include <utility>
#include <vector>

#include "../adapters/FusionWorkspaceProfileTypes.h"

namespace ChipCarving {
namespace Commands {

using ChipCarving::Adapters::CurveData;

/**
 * Log curve chaining info for debugging
 */
void logChainingInfo(size_t curveCount, bool hadTessellationIssues, double tolerance);

/**
 * Chain curves together and extract vertices in order
 * @param allCurves Vector of curve data to chain
 * @return Vector of (x,y) vertex pairs forming the chained polygon
 */
std::vector<std::pair<double, double>> chainCurvesAndExtractVertices(const std::vector<CurveData>& allCurves);

}  // namespace Commands
}  // namespace ChipCarving

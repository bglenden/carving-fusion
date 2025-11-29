/**
 * PluginCommandsGeometryChaining.cpp
 *
 * Geometry curve chaining implementation for PluginCommands
 * Part of PluginCommandsGeometry refactoring (Item #1a)
 * Extracted from PluginCommandsGeometry.cpp
 */

#include "PluginCommandsGeometryChaining.h"

#include "utils/logging.h"

namespace ChipCarving {
namespace Commands {

void logChainingInfo(size_t curveCount, bool hadTessellationIssues, double tolerance) {
  LOG_INFO("  Chaining " << curveCount << " curves...");
  if (hadTessellationIssues) {
    LOG_WARNING("  Using relaxed chaining tolerance " + std::to_string(tolerance) + " cm due to tessellation issues");
  }
}

std::vector<std::pair<double, double>> chainCurvesAndExtractVertices(const std::vector<CurveData>& allCurves) {
  std::vector<std::pair<double, double>> vertices;

  if (allCurves.empty()) {
    return vertices;
  }

  // Adaptive tolerance based on tessellation - use coarser tolerance if we
  // had issues
  double baseTolerance = 0.001;  // 0.01mm default tolerance

  // Check if we had any tessellation issues that might require looser
  // tolerance
  bool hadTessellationIssues = false;
  for (const auto& curve : allCurves) {
    if (curve.strokePoints.size() <= 2) {
      hadTessellationIssues = true;
      break;
    }
  }

  const double tolerance = hadTessellationIssues ? baseTolerance * 10 : baseTolerance;
  logChainingInfo(allCurves.size(), hadTessellationIssues, tolerance);

  // Start with first curve
  std::vector<size_t> chainOrder;
  chainOrder.push_back(0);
  std::vector<CurveData> tempCurves = allCurves;  // Make mutable copy
  tempCurves[0].used = true;
  auto currentEndPoint = tempCurves[0].endPoint;

  // Chain remaining curves
  for (size_t chainPos = 1; chainPos < tempCurves.size(); ++chainPos) {
    bool foundNext = false;

    for (size_t i = 0; i < tempCurves.size(); ++i) {
      if (tempCurves[i].used)
        continue;

      // Check if this curve's start connects to current end
      double distStart = std::sqrt(std::pow(currentEndPoint->x() - tempCurves[i].startPoint->x(), 2) +
                                   std::pow(currentEndPoint->y() - tempCurves[i].startPoint->y(), 2) +
                                   std::pow(currentEndPoint->z() - tempCurves[i].startPoint->z(), 2));

      // Check if this curve's end connects to current end (needs reversal)
      double distEnd = std::sqrt(std::pow(currentEndPoint->x() - tempCurves[i].endPoint->x(), 2) +
                                 std::pow(currentEndPoint->y() - tempCurves[i].endPoint->y(), 2) +
                                 std::pow(currentEndPoint->z() - tempCurves[i].endPoint->z(), 2));

      if (distStart < tolerance) {
        // Normal orientation
        chainOrder.push_back(i);
        tempCurves[i].used = true;
        currentEndPoint = tempCurves[i].endPoint;
        foundNext = true;
        LOG_INFO("    Chained curve " << i << " (normal)");
        break;
      } else if (distEnd < tolerance) {
        // Reversed orientation - mark with special flag
        chainOrder.push_back(i);
        tempCurves[i].used = true;
        currentEndPoint = tempCurves[i].startPoint;
        foundNext = true;
        LOG_INFO("    Chained curve " << i << " (reversed)");
        break;
      }
    }

    if (!foundNext) {
      LOG_ERROR("    Could not find connecting curve at position " << chainPos << " of " << tempCurves.size());
      LOG_ERROR("    Current endpoint: (" << currentEndPoint->x() << ", " << currentEndPoint->y() << ", "
                                          << currentEndPoint->z() << ")");

      // Log remaining unconnected curves for debugging
      int unconnectedCount = 0;
      for (size_t i = 0; i < tempCurves.size(); ++i) {
        if (!tempCurves[i].used) {
          LOG_ERROR("    Unconnected curve " << i << ": start(" << tempCurves[i].startPoint->x() << ", "
                                             << tempCurves[i].startPoint->y() << ") end(" << tempCurves[i].endPoint->x()
                                             << ", " << tempCurves[i].endPoint->y() << ")");
          unconnectedCount++;
        }
      }
      LOG_ERROR("    Total unconnected curves: " << unconnectedCount << " - profile will be incomplete");
      break;
    }
  }

  // Extract vertices from chained curves
  for (size_t i = 0; i < chainOrder.size(); ++i) {
    const CurveData& curve = tempCurves[chainOrder[i]];
    const auto& strokePoints = curve.strokePoints;

    // Determine how many points to add (skip last to avoid duplicates)
    size_t numPoints = strokePoints.size() - 1;

    // Check if reversed by comparing endpoints
    if (i < tempCurves.size() && std::abs(curve.endPoint->x() - currentEndPoint->x()) < tolerance / 2 &&
        std::abs(curve.endPoint->y() - currentEndPoint->y()) < tolerance / 2 &&
        std::abs(curve.endPoint->z() - currentEndPoint->z()) < tolerance / 2) {
      // Add points in reverse order
      for (int j = static_cast<int>(strokePoints.size()) - 1; j >= 1; --j) {
        if (strokePoints[j]) {
          vertices.push_back({strokePoints[j]->x(), strokePoints[j]->y()});
        }
      }
    } else {
      // Add points in normal order
      for (size_t j = 0; j < numPoints; ++j) {
        if (strokePoints[j]) {
          vertices.push_back({strokePoints[j]->x(), strokePoints[j]->y()});
        }
      }
    }
  }

  LOG_INFO("  Successfully chained curves, extracted " << vertices.size() << " vertices");

  return vertices;
}

}  // namespace Commands
}  // namespace ChipCarving

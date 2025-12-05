/**
 * MedialAxisUtilities.cpp
 *
 * Implementation of medial axis sampling utilities for CNC toolpath generation.
 */

#include "geometry/MedialAxisUtilities.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace ChipCarving {
namespace Geometry {

std::vector<SampledMedialPath> sampleMedialAxisPaths(const std::vector<std::vector<Point2D>>& chains,
                                                     const std::vector<std::vector<double>>& clearanceRadii,
                                                     double targetSpacing) {
  std::vector<SampledMedialPath> sampledPaths;

  // Validate input
  if (chains.size() != clearanceRadii.size()) {
    // Input mismatch - return empty result
    return sampledPaths;
  }

  // Process each chain independently
  for (size_t chainIdx = 0; chainIdx < chains.size(); ++chainIdx) {
    const auto& chain = chains[chainIdx];
    const auto& clearances = clearanceRadii[chainIdx];

    // Skip empty or mismatched chains
    if (chain.empty() || chain.size() != clearances.size()) {
      continue;
    }

    SampledMedialPath sampledPath;

    // Single point chain - just add it
    if (chain.size() == 1) {
      sampledPath.points.emplace_back(chain[0], clearances[0]);
      sampledPath.totalLength = 0.0;
      sampledPaths.push_back(sampledPath);
      continue;
    }

    // Calculate total chain length
    double totalLength = 0.0;
    for (size_t i = 1; i < chain.size(); ++i) {
      totalLength += distance(chain[i - 1], chain[i]);
    }
    sampledPath.totalLength = totalLength;

    // Build enhanced chain by interpolating long segments
    std::vector<Point2D> enhancedChain;
    std::vector<double> enhancedClearances;

    for (size_t i = 0; i < chain.size(); ++i) {
      // Always add the current point
      enhancedChain.push_back(chain[i]);
      enhancedClearances.push_back(clearances[i]);

      // If not the last point, check if interpolation is needed
      if (i < chain.size() - 1) {
        double segmentLength = distance(chain[i], chain[i + 1]);

        // Interpolate if segment is longer than 1.5mm
        if (segmentLength > 1.5) {
          int numIntermediatePoints = static_cast<int>(segmentLength / 1.0) - 1;

          // Safety limit to prevent excessive interpolation
          const int MAX_INTERMEDIATE_POINTS = 50;
          numIntermediatePoints = std::min(numIntermediatePoints, MAX_INTERMEDIATE_POINTS);

          for (int j = 1; j <= numIntermediatePoints; ++j) {
            double t = static_cast<double>(j) / static_cast<double>(numIntermediatePoints + 1);

            // Linear interpolation of position
            Point2D interpolatedPoint(chain[i].x + t * (chain[i + 1].x - chain[i].x),
                                      chain[i].y + t * (chain[i + 1].y - chain[i].y));

            // Linear interpolation of clearance
            double interpolatedClearance = clearances[i] + t * (clearances[i + 1] - clearances[i]);

            enhancedChain.push_back(interpolatedPoint);
            enhancedClearances.push_back(interpolatedClearance);
          }
        }
      }
    }

    // Build cumulative distance array for enhanced chain
    std::vector<double> cumulativeDistances(enhancedChain.size(), 0.0);
    for (size_t i = 1; i < enhancedChain.size(); ++i) {
      double segmentLength = distance(enhancedChain[i - 1], enhancedChain[i]);
      cumulativeDistances[i] = cumulativeDistances[i - 1] + segmentLength;
    }

    // Select points at evenly spaced distances
    std::vector<size_t> selectedIndices;
    std::set<size_t> selectedSet;

    // Always include start and end points
    selectedIndices.push_back(0);
    selectedSet.insert(0);
    if (enhancedChain.size() > 1) {
      selectedIndices.push_back(enhancedChain.size() - 1);
      selectedSet.insert(enhancedChain.size() - 1);
    }

    // Calculate number of target points based on path length
    int targetCount = std::max(2, static_cast<int>(totalLength / targetSpacing) + 1);

    if (targetCount > 2 && totalLength > 0.2) {
      // Add intermediate points at regular intervals
      for (int target = 1; target < targetCount - 1; ++target) {
        double targetDistance = (totalLength * target) / (targetCount - 1);

        // Find the point closest to this target distance
        size_t bestIndex = 0;
        double bestDifference = std::abs(cumulativeDistances[0] - targetDistance);

        for (size_t i = 1; i < enhancedChain.size(); ++i) {
          double difference = std::abs(cumulativeDistances[i] - targetDistance);
          if (difference < bestDifference) {
            bestDifference = difference;
            bestIndex = i;
          }
        }

        // Add if not already selected and not too close to existing points
        if (selectedSet.find(bestIndex) == selectedSet.end()) {
          bool tooClose = std::any_of(selectedSet.begin(), selectedSet.end(), [&](size_t selected) {
            return distance(enhancedChain[selected], enhancedChain[bestIndex]) < 0.1;
          });

          if (!tooClose) {
            selectedIndices.push_back(bestIndex);
            selectedSet.insert(bestIndex);
          }
        }
      }
    }

    // Sort selected indices
    std::sort(selectedIndices.begin(), selectedIndices.end());

    // Build the sampled path from selected points
    for (size_t idx : selectedIndices) {
      sampledPath.points.emplace_back(enhancedChain[idx], enhancedClearances[idx]);
    }

    sampledPaths.push_back(sampledPath);
  }

  return sampledPaths;
}

}  // namespace Geometry
}  // namespace ChipCarving

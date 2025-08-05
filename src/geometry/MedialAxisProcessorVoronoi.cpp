/**
 * MedialAxisProcessorVoronoi.cpp
 *
 * OpenVoronoi-specific operations for MedialAxisProcessor
 * Split from MedialAxisProcessor.cpp for maintainability
 */

#include "../../include/geometry/MedialAxisProcessor.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <streambuf>

#include "../../include/geometry/Shape.h"
#include "../../include/utils/logging.h"

// OpenVoronoi includes
#include <medial_axis_filter.hpp>
#include <medial_axis_walk.hpp>
#include <polygon_interior_filter.hpp>
#include <version.hpp>
#include <voronoidiagram.hpp>

namespace ChipCarving {
namespace Geometry {

std::vector<SampledMedialPath> MedialAxisProcessor::getSampledPaths(
    const MedialAxisResults& results, double spacing) {
    if (!results.success) {
        log("Warning: Cannot sample paths from failed medial axis computation");
        return {};
    }

    // The chains in results are already in world coordinates (cm from computeMedialAxisFromProfile)
    // We just need to convert from cm to mm for the sampling function
    std::vector<std::vector<Point2D>> worldChains;
    std::vector<std::vector<double>> worldClearances;

    log("Converting " + std::to_string(results.chains.size()) + " chains from world cm to world mm");

    for (size_t i = 0; i < results.chains.size(); ++i) {
        std::vector<Point2D> worldChain;
        std::vector<double> worldClearance;

        for (size_t j = 0; j < results.chains[i].size(); ++j) {
            // Results are already in world coordinates (cm), just convert to mm
            Point2D worldPointMm(results.chains[i][j].x * 10.0, results.chains[i][j].y * 10.0);
            worldChain.push_back(worldPointMm);

            // Clearance radius is also in world coordinates (cm), convert to mm
            double worldRadiusMm = results.clearanceRadii[i][j] * 10.0;
            worldClearance.push_back(worldRadiusMm);

            // Log first few points for debugging
            if (i == 0 && j < 3) {
                log("Chain 0, point " + std::to_string(j) + ": world cm (" +
                    std::to_string(results.chains[i][j].x) + ", " + std::to_string(results.chains[i][j].y) +
                    ") -> world mm (" + std::to_string(worldPointMm.x) + ", " + std::to_string(worldPointMm.y) +
                    "), clearance " + std::to_string(worldRadiusMm) + " mm");
            }
        }

        worldChains.push_back(worldChain);
        worldClearances.push_back(worldClearance);
    }

    return sampleMedialAxisPaths(worldChains, worldClearances, spacing);
}

bool MedialAxisProcessor::computeOpenVoronoi(const std::vector<Point2D>& transformedPolygon,
                                             MedialAxisResults& results) {

    try {
        // Validate polygon before processing
        if (!validatePolygonForOpenVoronoi(transformedPolygon)) {
            results.errorMessage = "Invalid polygon for OpenVoronoi processing";
            return false;
        }

        // Create VoronoiDiagram using smart pointer for automatic cleanup
        int numSites = static_cast<int>(transformedPolygon.size());
        int bins = std::max(10, static_cast<int>(std::sqrt(numSites)));
        auto vd = std::make_unique<ovd::VoronoiDiagram>(1.0, bins);

        // Don't enable debug mode - it's too verbose
        // vd->debug_on();

        log("OpenVoronoi version: " + std::string(ovd::version()));
        log("Processing polygon with " + std::to_string(numSites) + " vertices, using " +
            std::to_string(bins) + " bins");

        // Insert point sites
        // Following Fusion convention: polygons are implicitly closed, no duplicate vertices
        std::vector<int> pointIds;
        pointIds.reserve(numSites);

        for (size_t i = 0; i < transformedPolygon.size(); ++i) {
            const auto& point = transformedPolygon[i];
            ovd::Point ovdPoint(point.x, point.y);

            // Log before each point insertion
            if (verbose_) {
                LOG_DEBUG("About to insert point " << i << ": (" << point.x << ", " << point.y << ")");
            }

            int id = vd->insert_point_site(ovdPoint);

            // Log after successful insertion
            if (verbose_) {
                LOG_DEBUG("Successfully inserted point " << i << " with ID " << id);
            }

            pointIds.push_back(id);

            if (verbose_) {
                log("Added point " + std::to_string(id) + ": (" + std::to_string(point.x) + ", " +
                    std::to_string(point.y) + ")");
            }
        }

        // Insert line sites (connecting consecutive points)
        // Following Fusion convention: all polygons are implicitly closed
        size_t numLines = pointIds.size();
        log("Will insert " + std::to_string(numLines) + " line sites to form closed polygon");

        log("About to insert " + std::to_string(numLines) + " line sites");

        for (size_t i = 0; i < numLines; ++i) {
            int startId = pointIds[i];
            int endId;

            if (i == numLines - 1) {
                // Last line connects back to first
                endId = pointIds[0];
            } else {
                // Normal case: connect to next point
                endId = pointIds[i + 1];
            }

            // Log before line insertion
            if (verbose_) {
                LOG_DEBUG("About to insert line site " << i << ": " << startId << " -> " << endId);
            }

            log("Inserting line site " + std::to_string(i) + ": " + std::to_string(startId) +
                " -> " + std::to_string(endId));


            try {
                vd->insert_line_site(startId, endId);

                // Log after successful line insertion
                if (verbose_) {
                    LOG_DEBUG("Successfully inserted line site " << i);
                }

                log("Successfully inserted line site " + std::to_string(i));
            } catch (const std::exception& e) {
                log("ERROR inserting line site " + std::to_string(i) + ": " + e.what());
                throw;
            } catch (...) {
                log("ERROR inserting line site " + std::to_string(i) + ": unknown exception");
                throw;
            }
        }

        log("All line sites inserted successfully");

        // Validate the diagram
        log("About to validate Voronoi diagram...");
        bool isValid = vd->check();
        if (!isValid) {
            log("Warning: Voronoi diagram validation failed");
        } else {
            log("Voronoi diagram validated successfully");
        }

        // Apply filters - need to check polygon orientation first
        // Calculate signed area to determine winding order
        double signedArea = 0.0;
        size_t n = transformedPolygon.size();
        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n;
            signedArea += (transformedPolygon[j].x - transformedPolygon[i].x) *
                          (transformedPolygon[j].y + transformedPolygon[i].y);
        }
        signedArea /= 2.0;

        bool isCounterClockwise = signedArea > 0;
        log("Polygon winding order: " +
            std::string(isCounterClockwise ? "Counter-clockwise" : "Clockwise") +
            " (signed area: " + std::to_string(signedArea) + ")");

        // For CCW polygons, we want the interior (inside the polygon)
        // For CW polygons, we want the exterior to be filtered out
        ovd::polygon_interior_filter interiorFilter(!isCounterClockwise);
        vd->filter(&interiorFilter);

        ovd::medial_axis_filter medialFilter(medialThreshold_);
        vd->filter(&medialFilter);

        // Extract medial axis with configurable interpolation
        ovd::HEGraph& graph = vd->get_graph_reference();
        ovd::MedialAxisWalk walker(graph, medialAxisWalkPoints_);  // Configurable intermediate points
        ovd::MedialChainList chainList = walker.walk();

        // REMOVED: Enhancement of linear edges - we want to preserve exact OpenVoronoi vertices
        // to avoid double interpolation. Fusion will handle spline fitting through the raw vertices.

        log("Found " + std::to_string(chainList.size()) + " medial axis chains");

        // Convert results back to world coordinates
        results.numChains = static_cast<int>(chainList.size());
        results.minClearance = std::numeric_limits<double>::max();
        results.maxClearance = 0.0;

        for (const auto& chain : chainList) {
            std::vector<Point2D> worldChain;
            std::vector<double> worldClearances;

            for (const auto& pointList : chain) {
                for (const auto& medialPoint : pointList) {
                    // Convert point back to world coordinates
                    Point2D unitPoint(medialPoint.p.x, medialPoint.p.y);
                    Point2D worldPoint = transformFromUnitCircle(unitPoint, results.transform);
                    worldChain.push_back(worldPoint);

                    // Convert clearance radius back to world scale
                    double worldClearance = medialPoint.clearance_radius / results.transform.scale;
                    worldClearances.push_back(worldClearance);

                    // Update statistics
                    results.totalPoints++;
                    results.minClearance = std::min(results.minClearance, worldClearance);
                    results.maxClearance = std::max(results.maxClearance, worldClearance);

                    // Debug output for first few points only
                    if (verbose_ && results.totalPoints <= 3) {
                        log("Medial point: (" + std::to_string(worldPoint.x) + ", " +
                            std::to_string(worldPoint.y) +
                            "), clearance: " + std::to_string(worldClearance));
                    }
                }
            }

            if (!worldChain.empty()) {
                results.chains.push_back(worldChain);
                results.clearanceRadii.push_back(worldClearances);

                // Calculate chain length
                double chainLength = 0.0;
                for (size_t i = 1; i < worldChain.size(); ++i) {
                    chainLength += distance(worldChain[i - 1], worldChain[i]);
                }
                results.totalLength += chainLength;
            }
        }

        // Smart pointer automatically cleans up - no manual delete needed

        if (results.minClearance == std::numeric_limits<double>::max()) {
            results.minClearance = 0.0;
        }

        return true;

    } catch (const std::exception& e) {
        results.errorMessage = "OpenVoronoi computation failed: " + std::string(e.what());
        log("Error: " + results.errorMessage);
        return false;
    } catch (...) {
        results.errorMessage = "OpenVoronoi computation failed with unknown error";
        log("Error: " + results.errorMessage);
        return false;
    }
}

void MedialAxisProcessor::log(const std::string& message) {
    // Use ERROR level for error messages, INFO level for other messages
    if (message.find("ERROR") == 0 || message.find("Error") == 0) {
        LOG_ERROR("[MedialAxisProcessor] " << message);
    } else {
        LOG_INFO("[MedialAxisProcessor] " << message);
    }
}

}  // namespace Geometry
}  // namespace ChipCarving

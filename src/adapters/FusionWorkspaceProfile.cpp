/**
 * FusionWorkspaceProfile.cpp
 *
 * Profile and geometry extraction operations for FusionWorkspace
 * Split from FusionWorkspace.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "FusionAPIAdapter.h"
#include "FusionWorkspaceProfileTypes.h"
#include "../../include/utils/TempFileManager.h"
#include "../utils/DebugLogger.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

bool FusionWorkspace::extractProfileVertices(const std::string& entityId,
                                             std::vector<std::pair<double, double>>& vertices,
                                             TransformParams& transform) {
    // Enhanced UI Phase 5.2: Extract geometry from Fusion 360 sketch profiles
    // FIXED: This function now returns vertices in WORLD COORDINATES
    // for proper medial axis computation. Construction geometry is created
    // on the same sketch plane, so coordinate alignment works correctly.
    vertices.clear();

    // Use centralized debug logging
    auto logger = ChipCarving::Utils::DebugLogger::getInstance();
    logger->logSectionHeader("PROFILE EXTRACTION", "extractProfileVertices called for: " + entityId);
    logger->createIndicatorFile("extraction_called.txt", "extractProfileVertices called for: " + entityId);

    try {
        // Use extracted profile search method
        Ptr<adsk::fusion::Profile> profile = findProfileByEntityToken(entityId);
        if (!profile) {
            logger->logError("Could not find selected profile with entityId: " + entityId);
            return false;
        }

        logger->logDebug("Processing selected profile");

        // Extract curves from profile using extracted method
        std::vector<CurveData> allCurves;
        if (!extractCurvesFromProfile(profile, allCurves, transform)) {
            logger->logError("Failed to extract curves from profile");
            return false;
        }

        // Now chain the curves in the correct order to form a closed polygon
        logger->logDebug("Starting curve chaining algorithm...");

        std::vector<size_t> chainOrder;
        const double tolerance = 0.001;  // 0.01mm tolerance for point matching

        // Start with the first curve
        chainOrder.push_back(0);
        allCurves[0].used = true;

        Ptr<adsk::core::Point3D> currentEndPoint = allCurves[0].endPoint;
        logger->logDebug("Starting chain with curve 0, end point: (" + std::to_string(currentEndPoint->x()) + ", " + std::to_string(currentEndPoint->y()) + ")");

        // Find the next connected curve
        for (size_t chainPos = 1; chainPos < allCurves.size(); ++chainPos) {
            bool foundNext = false;

            for (size_t i = 0; i < allCurves.size(); ++i) {
                if (allCurves[i].used)
                    continue;

                // Check if this curve's start connects to our current end
                double distToStart =
                    std::sqrt(std::pow(allCurves[i].startPoint->x() - currentEndPoint->x(), 2) +
                              std::pow(allCurves[i].startPoint->y() - currentEndPoint->y(), 2));

                // Check if this curve's end connects to our current end (reverse direction)
                double distToEnd =
                    std::sqrt(std::pow(allCurves[i].endPoint->x() - currentEndPoint->x(), 2) +
                              std::pow(allCurves[i].endPoint->y() - currentEndPoint->y(), 2));

                if (distToStart < tolerance) {
                    // Normal direction connection
                    chainOrder.push_back(i);
                    allCurves[i].used = true;
                    currentEndPoint = allCurves[i].endPoint;
                    foundNext = true;
                    logger->logDebug("Chained curve " + std::to_string(i) + " (normal), end point: (" + std::to_string(currentEndPoint->x()) + ", " + std::to_string(currentEndPoint->y()) + ")");
                    break;
                } else if (distToEnd < tolerance) {
                    // Reverse direction connection - need to reverse the stroke points
                    chainOrder.push_back(i | 0x80000000);  // Mark as reversed with high bit
                    allCurves[i].used = true;
                    currentEndPoint = allCurves[i].startPoint;
                    foundNext = true;
                    logger->logDebug("Chained curve " + std::to_string(i) + " (REVERSED), end point: (" + std::to_string(currentEndPoint->x()) + ", " + std::to_string(currentEndPoint->y()) + ")");
                    break;
                }
            }

            if (!foundNext) {
                logger->logWarning("Could not find next curve to chain at position " + std::to_string(chainPos));
                break;
            }
        }

        std::string chainOrderStr = "Chaining complete. Order: ";
        for (size_t idx : chainOrder) {
            bool reversed = (idx & 0x80000000) != 0;
            size_t curveIdx = idx & 0x7FFFFFFF;
            chainOrderStr += std::to_string(curveIdx) + (reversed ? "R" : "") + " ";
        }
        logger->logDebug(chainOrderStr);

        // Now build the final vertex list from the chained curves
        for (size_t i = 0; i < chainOrder.size(); ++i) {
            bool reversed = (chainOrder[i] & 0x80000000) != 0;
            size_t curveIdx = chainOrder[i] & 0x7FFFFFFF;

            const auto& curveData = allCurves[curveIdx];
            const auto& strokePoints = curveData.strokePoints;

            logger->logDebug("Adding curve " + std::to_string(curveIdx) + (reversed ? " (reversed)" : "") + " with " + std::to_string(strokePoints.size()) + " points");

            // Determine how many points to add (always skip last point to avoid duplicates)
            // For closed polygons, we don't want to duplicate the final vertex
            size_t numPoints = strokePoints.size() - 1;

            if (reversed) {
                // Add points in reverse order, skip first point (which is last in reverse) to avoid
                // duplicates
                for (int j = strokePoints.size() - 1; j >= 1; --j) {
                    if (strokePoints[j]) {
                        double x = strokePoints[j]->x();
                        double y = strokePoints[j]->y();
                        double z = strokePoints[j]->z();

                        // NOTE: With world coordinates, Z can be non-zero (expected)
                        // This is the correct behavior for medial axis computation
                        if (std::abs(z) > 0.001) {
                            logger->logDebug("Point has Z value: " + std::to_string(z) + " cm (world coordinates)");
                        }

                        vertices.push_back({x, y});
                    }
                }
            } else {
                // Add points in normal order
                for (size_t j = 0; j < numPoints; ++j) {
                    if (strokePoints[j]) {
                        double x = strokePoints[j]->x();
                        double y = strokePoints[j]->y();
                        double z = strokePoints[j]->z();

                        // NOTE: With world coordinates, Z can be non-zero (expected)
                        // This is the correct behavior for medial axis computation
                        if (std::abs(z) > 0.001) {
                            logger->logDebug("Point has Z value: " + std::to_string(z) + " cm (world coordinates)");
                        }

                        vertices.push_back({x, y});
                    }
                }
            }
        }

        logger->logDebug("Final chained polygon has " + std::to_string(vertices.size()) + " vertices");

        if (vertices.empty()) {
            logger->logError("No vertices in final chained polygon");
                return false;
        }

        // DO NOT transform vertices here - MedialAxisProcessor will handle all transformations
        // Just store dummy transform parameters since they're required by the interface
        transform.centerX = 0.0;
        transform.centerY = 0.0;
        transform.scale = 1.0;

        logger->logInfo("Extracted " + std::to_string(vertices.size()) + " vertices from real profile (in world coordinates cm)");

        // Generate SVG debug output for Fusion strokes
        logger->logDebug("About to generate Fusion strokes SVG with " + std::to_string(vertices.size()) + " vertices");

        // First, try creating a simple test file to verify write permissions
        std::string testFilePath = chip_carving::TempFileManager::getLogFilePath("fusion_strokes_test.txt");
        std::ofstream testFile(testFilePath);
        if (testFile.is_open()) {
            testFile << "SVG generation test - " << vertices.size() << " vertices extracted"
                     << std::endl;
            testFile.close();
            logger->logDebug("Test file created successfully");
        } else {
            logger->logError("Cannot create test file - permissions issue?");
        }

        try {
            // Generate timestamp for unique filename
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            std::string svgFileName = "fusion_strokes_debug_" + std::to_string(time_t) + ".svg";
            std::string svgFilePath = chip_carving::TempFileManager::getSVGFilePath(svgFileName);

            std::ofstream svgFile(svgFilePath);
            if (svgFile.is_open()) {
                logger->logDebug("Writing Fusion strokes SVG to: " + svgFilePath);

                // Calculate bounding box for viewport
                if (!vertices.empty()) {
                    double minX = vertices[0].first, maxX = vertices[0].first;
                    double minY = vertices[0].second, maxY = vertices[0].second;

                    for (const auto& vertex : vertices) {
                        minX = std::min(minX, vertex.first);
                        maxX = std::max(maxX, vertex.first);
                        minY = std::min(minY, vertex.second);
                        maxY = std::max(maxY, vertex.second);
                    }

                    // Add margin
                    double margin = std::max(maxX - minX, maxY - minY) * 0.1;
                    minX -= margin;
                    maxX += margin;
                    minY -= margin;
                    maxY += margin;

                    // SVG header with proper viewport
                    svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                    svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
                    svgFile << "width=\"800\" height=\"600\" ";
                    svgFile << "viewBox=\"" << minX << " " << minY << " " << (maxX - minX) << " "
                            << (maxY - minY) << "\">\n";
                    svgFile << "<title>Fusion Strokes Debug - " << entityId << "</title>\n";

                    // Flip Y coordinate system (SVG Y increases downward, but our coordinates
                    // increase upward)
                    svgFile << "<g transform=\"scale(1,-1)\">\n";

                    // Draw the polygon from Fusion strokes
                    svgFile << "<polygon points=\"";
                    for (size_t i = 0; i < vertices.size(); ++i) {
                        if (i > 0)
                            svgFile << " ";
                        svgFile << vertices[i].first << "," << vertices[i].second;
                    }
                    svgFile << "\" fill=\"none\" stroke=\"blue\" stroke-width=\"0.05\" />\n";

                    // Mark vertices with small circles and numbers
                    for (size_t i = 0; i < vertices.size(); ++i) {
                        svgFile << "<circle cx=\"" << vertices[i].first << "\" cy=\""
                                << vertices[i].second << "\" r=\"0.1\" fill=\"red\" />\n";

                        // Add vertex number (flip text back)
                        svgFile << "<text x=\"" << vertices[i].first + 0.15 << "\" y=\""
                                << vertices[i].second + 0.05
                                << "\" font-size=\"0.3\" fill=\"black\" transform=\"scale(1,-1)\">"
                                << i << "</text>\n";
                    }

                    svgFile << "</g>\n";
                    svgFile << "</svg>\n";

                    logger->logDebug("Fusion strokes SVG written successfully with " + std::to_string(vertices.size()) + " vertices");
                } else {
                    svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                    svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" "
                               "height=\"400\">\n";
                    svgFile << "<text x=\"50\" y=\"200\" font-size=\"20\" fill=\"red\">NO VERTICES "
                               "EXTRACTED</text>\n";
                    svgFile << "</svg>\n";
                    logger->logError("No vertices to write to SVG");
                }

                svgFile.close();
            } else {
                logger->logError("Could not create SVG file: " + svgFilePath);
            }
        } catch (const std::exception& e) {
            logger->logError("Exception creating Fusion strokes SVG: " + std::string(e.what()));
        } catch (...) {
            logger->logError("Unknown exception creating Fusion strokes SVG");
        }


        return true;

    } catch (const std::exception& e) {
        logger->logError("Exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        logger->logError("Unknown exception");
        return false;
    }
}

}  // namespace Adapters
}  // namespace ChipCarving

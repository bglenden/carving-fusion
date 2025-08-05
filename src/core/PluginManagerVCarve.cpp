/**
 * PluginManagerVCarve.cpp
 *
 * V-carve toolpath generation functionality for PluginManager
 * Split from PluginManagerPaths.cpp for maintainability
 */

#include "PluginManager.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>

#include "../../include/geometry/Point2D.h"
#include "../../include/geometry/Point3D.h"
#include "../../include/geometry/VCarveCalculator.h"
#include "../../include/utils/logging.h"
#include "../utils/UnitConversion.h"

using namespace ChipCarving::Utils;

namespace ChipCarving {
namespace Core {

bool PluginManager::generateVCarveToolpaths(const std::vector<Geometry::MedialAxisResults>& medialResults,
                                           const Adapters::MedialAxisParameters& params,
                                           Adapters::ISketch* sketch,
                                           const std::vector<Adapters::IWorkspace::TransformParams>& transforms) {
    if (!sketch || medialResults.empty()) {
        return false;
    }

    if (transforms.size() != medialResults.size()) {
        return false;
    }

    try {
        // NOTE: Not applying any coordinate transformations
        // Fusion handles the transformation when creating sketch entities on the correct plane

        // Create V-carve calculator
        Geometry::VCarveCalculator calculator;

        int totalVCarvePaths = 0;

        // Process each medial axis result independently
        for (size_t i = 0; i < medialResults.size(); ++i) {
            const auto& medialResult = medialResults[i];
            const auto& transform = transforms[i];  // Get corresponding transform for this profile

            if (!medialResult.success || medialResult.chains.empty()) {
                continue;
            }

            // Get sketch plane Z in mm (transform stores it in cm)
            double sketchPlaneZ_mm = transform.sketchPlaneZ * 10.0;

            Geometry::VCarveResults vcarveResults;

            // Check if surface projection is needed
            if (params.projectToSurface && !params.targetSurfaceId.empty() && workspace_) {

                // Create surface query function that calls workspace
                // FIXED: Medial axis coordinates are already in cm, don't convert again
                auto surfaceQuery = [this, &params](double x_cm, double y_cm) -> double {
                    // Coordinates are already in cm, no conversion needed
                    double z_cm = workspace_->getSurfaceZAtXY(params.targetSurfaceId, x_cm, y_cm);
                    // Convert result back to mm for V-carve calculator
                    double z_mm = z_cm * 10.0;

                    // FIXED: Debug logging to trace surface query conversion
                    static int queryCount = 0;
                    if (queryCount < 3) {
                        LOG_DEBUG("[SURFACE QUERY TRACE] Query " << queryCount << ": (" << x_cm << ", " << y_cm 
                                  << ") cm -> z_cm=" << z_cm << " cm -> z_mm=" << z_mm << " mm");
                        queryCount++;
                    }

                    return z_mm;
                };

                // Generate sampled paths for this specific medial result
                auto sampledPaths = medialProcessor_->getSampledPaths(medialResult, params.samplingDistance);

                // Generate V-carve paths using sampled medial axis paths for better surface following
                // This uses the user-specified sampling distance for uniform spacing
                vcarveResults = calculator.generateVCarvePaths(sampledPaths, params);

                // Apply surface projection to the V-carve points
                for (auto& vcarvePath : vcarveResults.paths) {
                    for (auto& vcarvePoint : vcarvePath.points) {
                        // Query surface Z at this XY position
                        // FIXED: Convert mm coordinates to cm for surface query
                        double x_cm = vcarvePoint.position.x / 10.0;
                        double y_cm = vcarvePoint.position.y / 10.0;
                        double surfaceZ_mm = surfaceQuery(x_cm, y_cm);

                        // FIXED: Debug logging for surface Z storage
                        static int storeCount = 0;
                        if (storeCount < 3) {
                            LOG_DEBUG("[SURFACE STORE TRACE] Store " << storeCount << ": position(" 
                                      << vcarvePoint.position.x << ", " << vcarvePoint.position.y 
                                      << ") mm -> surfaceZ_mm=" << surfaceZ_mm << " mm");
                            storeCount++;
                        }

                        if (!std::isnan(surfaceZ_mm)) {
                            // Store the original depth (how deep below sketch plane)
                            double originalDepth = vcarvePoint.depth;

                            // For surface projection, we need to store how far below the surface to carve
                            // NOT the absolute Z position

                            // Store the original carve depth with a marker to indicate surface projection
                            // We use a large negative offset to distinguish from regular depths
                            const double SURFACE_PROJECTION_MARKER = -1000000.0;
                            vcarvePoint.depth = SURFACE_PROJECTION_MARKER - originalDepth;

                            // Also store the surface Z for later use
                            // We'll use the clearanceRadius field temporarily since we're already calculating it
                            vcarvePoint.clearanceRadius = surfaceZ_mm;
                        }
                    }
                }
            } else {
                // Generate sampled paths for this specific medial result
                auto sampledPaths = medialProcessor_->getSampledPaths(medialResult, params.samplingDistance);

                // Generate V-carve paths using sampled medial axis paths for uniform spacing
                vcarveResults = calculator.generateVCarvePaths(sampledPaths, params);
            }

            if (!vcarveResults.success) {
                continue;
            }

            // Add V-carve paths to sketch as 3D splines
            for (const auto& vcarvePath : vcarveResults.paths) {
                if (!vcarvePath.isValid()) {
                    continue;
                }

                // Convert VCarvePoints to Point3D for spline creation
                std::vector<Geometry::Point3D> splinePoints;
                splinePoints.reserve(vcarvePath.points.size());

                for (const auto& vcarvePoint : vcarvePath.points) {
                    // V-carve points are already in world coordinates (mm)
                    double x_world_mm = vcarvePoint.position.x;
                    double y_world_mm = vcarvePoint.position.y;

                    // Calculate Z coordinate
                    // IMPORTANT: Fusion interprets 3D sketch Z coordinates as RELATIVE to the sketch plane
                    double z_sketch_relative_mm;

                    const double SURFACE_PROJECTION_MARKER = -1000000.0;
                    if (vcarvePoint.depth < SURFACE_PROJECTION_MARKER + 1000.0) {
                        // This is a surface projection point
                        // Extract the carve depth from the encoded value
                        double carveDepth = -(vcarvePoint.depth - SURFACE_PROJECTION_MARKER);
                        double surfaceZ_mm = vcarvePoint.clearanceRadius;  // Temporarily stored here

                        // Calculate the target Z position (surfaceZ - carveDepth)
                        double targetZ_mm = surfaceZ_mm - carveDepth;

                        // Convert to sketch-relative coordinates
                        z_sketch_relative_mm = targetZ_mm - sketchPlaneZ_mm;

                        // FIXED: Debug logging to understand V-carve positioning
                        static int debugCount = 0;
                        if (debugCount < 5) {
                            LOG_DEBUG("[VCARVE DEBUG] Point " << debugCount << ": surfaceZ=" << surfaceZ_mm 
                                      << "mm, carveDepth=" << carveDepth << "mm, targetZ=" << targetZ_mm 
                                      << "mm, sketchPlaneZ=" << sketchPlaneZ_mm << "mm, z_relative=" << z_sketch_relative_mm << "mm");
                            debugCount++;
                        }
                    } else {
                        // Regular depth, carve below the sketch plane
                        z_sketch_relative_mm = -vcarvePoint.depth;
                    }

                    // Create 3D point with sketch-relative coordinates
                    // XY are in world coordinates, Z is relative to sketch plane
                    Geometry::Point3D point3D(x_world_mm, y_world_mm, z_sketch_relative_mm);
                    splinePoints.push_back(point3D);
                }

                // Add 3D spline to sketch
                if (splinePoints.size() >= 2) {
                    bool success = sketch->addSpline3D(splinePoints);
                    if (!success) {
                        logger_->logWarning("Failed to add V-carve 3D spline to sketch");
                    }
                } else {
                    logger_->logWarning("V-carve path has insufficient points for spline creation");
                }
            }

            // Update totals
            totalVCarvePaths += vcarveResults.totalPaths;
        }

        if (totalVCarvePaths > 0) {
            return true;
        } else {
            return false;
        }

    } catch (const std::exception& e) {
        logger_->logError("Exception in generateVCarveToolpaths: " + std::string(e.what()));
        return false;
    } catch (...) {
        logger_->logError("Unknown exception in generateVCarveToolpaths");
        return false;
    }
}

}  // namespace Core
}  // namespace ChipCarving

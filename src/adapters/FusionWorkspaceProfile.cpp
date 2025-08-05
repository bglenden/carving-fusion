/**
 * FusionWorkspaceProfile.cpp
 *
 * Profile and geometry extraction operations for FusionWorkspace
 * Split from FusionWorkspace.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#include "FusionAPIAdapter.h"
#include "FusionWorkspaceProfileTypes.h"
#include "../../include/utils/logging.h"

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
    LOG_DEBUG("=== PROFILE EXTRACTION === extractProfileVertices called for: " << entityId);
    // TODO(developer): Remove indicator file creation

    try {
        // Use extracted profile search method
        Ptr<adsk::fusion::Profile> profile = findProfileByEntityToken(entityId);
        if (!profile) {
            LOG_ERROR("Could not find selected profile with entityId: " << entityId);
            return false;
        }

        LOG_DEBUG("Processing selected profile");

        // Extract curves from profile using extracted method
        std::vector<CurveData> allCurves;
        if (!extractCurvesFromProfile(profile, allCurves, transform)) {
            LOG_ERROR("Failed to extract curves from profile");
            return false;
        }

        // Now chain the curves in the correct order to form a closed polygon
        LOG_DEBUG("Starting curve chaining algorithm...");

        std::vector<size_t> chainOrder;
        const double tolerance = 0.001;  // 0.01mm tolerance for point matching

        // Start with the first curve
        chainOrder.push_back(0);
        allCurves[0].used = true;

        Ptr<adsk::core::Point3D> currentEndPoint = allCurves[0].endPoint;
        LOG_DEBUG("Starting chain with curve 0, end point: (" << currentEndPoint->x() << ", " << currentEndPoint->y() << ")");

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
                    LOG_DEBUG("Chained curve " << i << " (normal), end point: (" << currentEndPoint->x() << ", " << currentEndPoint->y() << ")");
                    break;
                } else if (distToEnd < tolerance) {
                    // Reverse direction connection - need to reverse the stroke points
                    chainOrder.push_back(i | 0x80000000);  // Mark as reversed with high bit
                    allCurves[i].used = true;
                    currentEndPoint = allCurves[i].startPoint;
                    foundNext = true;
                    LOG_DEBUG("Chained curve " << i << " (REVERSED), end point: (" << currentEndPoint->x() << ", " << currentEndPoint->y() << ")");
                    break;
                }
            }

            if (!foundNext) {
                LOG_WARNING("Could not find next curve to chain at position " << chainPos);
                break;
            }
        }

        std::string chainOrderStr = "Chaining complete. Order: ";
        for (size_t idx : chainOrder) {
            bool reversed = (idx & 0x80000000) != 0;
            size_t curveIdx = idx & 0x7FFFFFFF;
            chainOrderStr += std::to_string(curveIdx) + (reversed ? "R" : "") + " ";
        }
        LOG_DEBUG(chainOrderStr);

        // Now build the final vertex list from the chained curves
        for (size_t i = 0; i < chainOrder.size(); ++i) {
            bool reversed = (chainOrder[i] & 0x80000000) != 0;
            size_t curveIdx = chainOrder[i] & 0x7FFFFFFF;

            const auto& curveData = allCurves[curveIdx];
            const auto& strokePoints = curveData.strokePoints;

            LOG_DEBUG("Adding curve " << curveIdx << (reversed ? " (reversed)" : "") << " with " << strokePoints.size() << " points");

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
                            LOG_DEBUG("Point has Z value: " << z << " cm (world coordinates)");
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
                            LOG_DEBUG("Point has Z value: " << z << " cm (world coordinates)");
                        }

                        vertices.push_back({x, y});
                    }
                }
            }
        }

        LOG_DEBUG("Final chained polygon has " << vertices.size() << " vertices");

        if (vertices.empty()) {
            LOG_ERROR("No vertices in final chained polygon");
                return false;
        }

        // DO NOT transform vertices here - MedialAxisProcessor will handle all transformations
        // Just store dummy transform parameters since they're required by the interface
        transform.centerX = 0.0;
        transform.centerY = 0.0;
        transform.scale = 1.0;

        LOG_DEBUG("Extracted " << vertices.size() << " vertices from real profile (in world coordinates cm)");

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Exception: " << e.what());
        return false;
    } catch (...) {
        LOG_ERROR("Unknown exception");
        return false;
    }
}

bool FusionWorkspace::extractProfileGeometry(Ptr<adsk::fusion::Profile> profile,
                                            ProfileGeometry& geometry) {
    try {
        if (!profile) {
            LOG_ERROR("Null profile provided to extractProfileGeometry");
            return false;
        }

        // Clear the geometry structure
        geometry.vertices.clear();
        geometry.area = 0.0;
        geometry.centroid = {0.0, 0.0};
        
        // Get area properties
        Ptr<adsk::fusion::AreaProperties> areaProps = profile->areaProperties();
        if (areaProps) {
            geometry.area = areaProps->area();
            Ptr<adsk::core::Point3D> centroidPt = areaProps->centroid();
            if (centroidPt) {
                geometry.centroid = {centroidPt->x(), centroidPt->y()};
            }
        }
        
        // Get the parent sketch
        Ptr<adsk::fusion::ProfileLoop> firstLoop = profile->profileLoops()->item(0);
        if (!firstLoop) {
            LOG_ERROR("Profile has no loops");
            return false;
        }
        
        Ptr<adsk::fusion::ProfileCurve> firstCurve = firstLoop->profileCurves()->item(0);
        if (!firstCurve || !firstCurve->sketchEntity()) {
            LOG_ERROR("Profile has no valid curves");
            return false;
        }
        
        Ptr<adsk::fusion::Sketch> sketch = firstCurve->sketchEntity()->parentSketch();
        if (sketch) {
            geometry.sketchName = sketch->name();
            
            // Get plane entity ID
            Ptr<adsk::core::Base> referenceEntity = sketch->referencePlane();
            if (referenceEntity) {
                Ptr<adsk::fusion::ConstructionPlane> constructionPlane = referenceEntity;
                if (constructionPlane) {
                    geometry.planeEntityId = constructionPlane->entityToken();
                } else {
                    Ptr<adsk::fusion::BRepFace> face = referenceEntity;
                    if (face) {
                        geometry.planeEntityId = face->entityToken();
                    }
                }
            }
        }
        
        // Extract all curves using existing method
        std::vector<CurveData> allCurves;
        IWorkspace::TransformParams transform;
        if (!extractCurvesFromProfile(profile, allCurves, transform)) {
            LOG_ERROR("Failed to extract curves from profile");
            return false;
        }
        geometry.transform = transform;
        
        // Chain curves and extract vertices (similar to existing extractProfileVertices)
        const double tolerance = 0.001;  // 0.01mm tolerance
        std::vector<size_t> chainOrder;
        
        // Start with first curve
        chainOrder.push_back(0);
        allCurves[0].used = true;
        Ptr<adsk::core::Point3D> currentEndPoint = allCurves[0].endPoint;
        
        // Chain remaining curves
        for (size_t chainPos = 1; chainPos < allCurves.size(); ++chainPos) {
            bool foundNext = false;
            
            for (size_t i = 0; i < allCurves.size(); ++i) {
                if (allCurves[i].used) continue;
                
                double distToStart = std::sqrt(
                    std::pow(allCurves[i].startPoint->x() - currentEndPoint->x(), 2) +
                    std::pow(allCurves[i].startPoint->y() - currentEndPoint->y(), 2));
                
                double distToEnd = std::sqrt(
                    std::pow(allCurves[i].endPoint->x() - currentEndPoint->x(), 2) +
                    std::pow(allCurves[i].endPoint->y() - currentEndPoint->y(), 2));
                
                if (distToStart < tolerance) {
                    chainOrder.push_back(i);
                    allCurves[i].used = true;
                    currentEndPoint = allCurves[i].endPoint;
                    foundNext = true;
                    break;
                } else if (distToEnd < tolerance) {
                    chainOrder.push_back(i | 0x80000000);  // Mark as reversed with high bit
                    allCurves[i].used = true;
                    currentEndPoint = allCurves[i].startPoint;
                    foundNext = true;
                    break;
                }
            }
            
            if (!foundNext) {
                LOG_ERROR("Failed to chain curves - gap in profile");
                return false;
            }
        }
        
        // Extract vertices from chained curves
        for (size_t i = 0; i < chainOrder.size(); ++i) {
            bool reversed = (chainOrder[i] & 0x80000000) != 0;
            size_t curveIdx = chainOrder[i] & 0x7FFFFFFF;
            
            const CurveData& curve = allCurves[curveIdx];
            const auto& strokePoints = curve.strokePoints;
            
            // Determine how many points to add (skip last to avoid duplicates)
            size_t numPoints = strokePoints.size() - 1;
            
            if (reversed) {
                // Add points in reverse order, skip first point (which is last in reverse)
                for (int j = strokePoints.size() - 1; j >= 1; --j) {
                    if (strokePoints[j]) {
                        geometry.vertices.push_back({strokePoints[j]->x(), strokePoints[j]->y()});
                    }
                }
            } else {
                // Add points in normal order
                for (size_t j = 0; j < numPoints; ++j) {
                    if (strokePoints[j]) {
                        geometry.vertices.push_back({strokePoints[j]->x(), strokePoints[j]->y()});
                    }
                }
            }
        }
        
        LOG_DEBUG("Extracted ProfileGeometry with " << geometry.vertices.size() 
                  << " vertices, area=" << geometry.area << " sq cm");
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in extractProfileGeometry: " << e.what());
        return false;
    } catch (...) {
        LOG_ERROR("Unknown exception in extractProfileGeometry");
        return false;
    }
}

}  // namespace Adapters
}  // namespace ChipCarving

/**
 * FusionWorkspaceCurveExtraction.cpp
 *
 * Curve extraction and tessellation operations for FusionWorkspace
 * Split from FusionWorkspaceProfile.cpp for maintainability
 */

#include "FusionAPIAdapter.h"
#include "FusionWorkspaceProfileTypes.h"
#include "../utils/DebugLogger.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

bool FusionWorkspace::extractCurvesFromProfile(
    adsk::core::Ptr<adsk::fusion::Profile> profile,
    std::vector<CurveData>& allCurves,
    TransformParams& transform) {

    auto logger = ChipCarving::Utils::DebugLogger::getInstance();
    logger->logDebug("Starting curve extraction from profile");

    if (!profile) {
        logger->logError("Null profile provided for curve extraction");
        return false;
    }

    // Verify sketch plane orientation (should be parallel to XY plane)
    try {
        // Get the parent sketch from the profile
        Ptr<adsk::fusion::Sketch> profileSketch = profile->parentSketch();
        if (profileSketch) {
            logger->logDebug("Got parent sketch for profile");

            // Check if the sketch curves are planar
            // Fusion 360 sketches are always on a plane, but we can verify the curves are 2D
            Ptr<adsk::fusion::SketchCurves> allSketchCurves = profileSketch->sketchCurves();
            if (allSketchCurves) {
                logger->logDebug("Sketch has " + std::to_string(allSketchCurves->count()) + " curves");

                // Check if any curve is explicitly 3D (not supported for our use case)
                bool has3DCurves = false;
                for (size_t i = 0; i < allSketchCurves->count() && i < 5; ++i) {  // Check first 5 curves
                    Ptr<adsk::fusion::SketchCurve> curve = allSketchCurves->item(i);
                    if (curve) {
                        // Get the curve's is2D property if it exists
                        // For now, just log the curve type
                        try {
                            if (curve->objectType() && std::string(curve->objectType()).find("3D") != std::string::npos) {
                                has3DCurves = true;
                                logger->logWarning("Found 3D curve at index " + std::to_string(i));
                            }
                        } catch (...) {
                            // Ignore property access errors
                        }
                    }
                }

                if (has3DCurves) {
                    logger->logWarning("Sketch contains 3D curves. Results may be unexpected.");
                } else {
                    logger->logDebug("All checked curves appear to be 2D (planar)");
                }
            }

            // Get the sketch plane to verify orientation
            Ptr<adsk::core::Base> refPlane = profileSketch->referencePlane();
            if (refPlane) {
                logger->logDebug("Got reference plane for sketch");

                // Try to get the geometry of the plane
                // The reference plane could be a construction plane, datum plane, etc.
                Ptr<adsk::fusion::ConstructionPlane> constructionPlane = refPlane;
                if (constructionPlane) {
                    Ptr<adsk::core::Plane> plane = constructionPlane->geometry();
                    if (plane) {
                        Ptr<adsk::core::Vector3D> normal = plane->normal();
                        if (normal) {
                            logger->logDebug("Sketch plane normal: (" + std::to_string(normal->x()) + ", " + std::to_string(normal->y()) + ", " + std::to_string(normal->z()) + ")");

                            // Check if the sketch plane is parallel to XY plane (normal close to Z-axis)
                            if (std::abs(normal->z()) > 0.99) {
                                if (normal->z() > 0) {
                                    logger->logDebug("Sketch plane is correctly oriented (parallel to XY plane)");
                                } else {
                                    logger->logDebug("Sketch plane is parallel to XY plane but pointing down");
                                }

                                // Get the sketch plane Z position
                                Ptr<adsk::core::Point3D> origin = plane->origin();
                                if (origin) {
                                    transform.sketchPlaneZ = origin->z();
                                    logger->logDebug("Sketch plane Z position: " + std::to_string(transform.sketchPlaneZ) + " cm");
                                }
                            }
                        }
                    }
                } else {
                    logger->logDebug("Reference plane is not a construction plane");
                }
            }
        }
    } catch (const std::exception& e) {
        logger->logWarning("Exception checking sketch plane: " + std::string(e.what()));
    } catch (...) {
        logger->logWarning("Unknown exception checking sketch plane");
    }

    // Get the loops from the profile (usually just one outer loop)
    Ptr<adsk::fusion::ProfileLoops> loops = profile->profileLoops();
    if (!loops || loops->count() == 0) {
        logger->logError("No loops in profile");
        return false;
    }
    logger->logDebug("Found " + std::to_string(loops->count()) + " loops");

    // Use the first loop (outer loop)
    Ptr<adsk::fusion::ProfileLoop> loop = loops->item(0);
    if (!loop) {
        logger->logError("Could not get profile loop");
        return false;
    }

    // Get the curves in the loop
    Ptr<adsk::fusion::ProfileCurves> profileCurves = loop->profileCurves();
    if (!profileCurves || profileCurves->count() == 0) {
        logger->logError("No curves in profile loop");
        return false;
    }
    logger->logDebug("Found " + std::to_string(profileCurves->count()) + " curves in loop");

    // Check if the profile loop is closed
    bool isClosedLoop = loop->isOuter();  // Outer loops are typically closed
    logger->logDebug("Profile loop is " + std::string(isClosedLoop ? "closed" : "open"));

    // Tessellate each curve in the loop using getStrokes
    double chordTolerance = 0.01;  // 0.1mm tolerance (Fusion uses cm units)

    // CRITICAL FIX: Curves come in arbitrary order from Fusion, not connected order
    // We need to chain them properly to form a closed polygon
    logger->logDebug("Found " + std::to_string(profileCurves->count()) + " curves - need to chain them in order");

    // First, collect all curves with their stroke points and endpoints
    allCurves.clear();
    allCurves.reserve(profileCurves->count());

    for (size_t i = 0; i < profileCurves->count(); ++i) {
        CurveData curveData;
        curveData.originalIndex = i;
        curveData.used = false;

        Ptr<adsk::fusion::ProfileCurve> profileCurve = profileCurves->item(i);
        if (!profileCurve) {
            logger->logWarning("Null profile curve at index " + std::to_string(i));
            continue;
        }

        // Get the underlying sketch entity
        Ptr<adsk::fusion::SketchEntity> sketchEntity = profileCurve->sketchEntity();
        if (!sketchEntity) {
            logger->logWarning("No sketch entity for curve " + std::to_string(i));
            continue;
        }

        // Cast to SketchCurve to get geometry
        Ptr<adsk::fusion::SketchCurve> sketchCurve = sketchEntity;
        if (!sketchCurve) {
            logger->logWarning("Could not cast to SketchCurve for curve " + std::to_string(i));
            continue;
        }

        // Log detailed information about the curve type
        logger->logDebug("Curve " + std::to_string(i) + " - SketchCurve type: " + std::string(sketchCurve->objectType()));

        // Check what this object actually is and what methods it has
        if (Ptr<adsk::fusion::SketchLine> line = sketchCurve) {
            logger->logDebug("Curve " + std::to_string(i) + " IS a SketchLine");
            // Try to get basic properties
            try {
                auto geom = line->geometry();
                logger->logDebug("SketchLine.geometry() returned: " + std::string(geom ? "valid pointer" : "NULL"));
                if (geom) {
                    logger->logDebug("Line geometry type: " + std::string(geom->objectType()));
                }
            } catch (...) {
                logger->logError("Exception calling SketchLine.geometry()");
            }
        } else {
            logger->logDebug("Curve " + std::to_string(i) + " is NOT a SketchLine (casting failed)");
        }

        // Get the 3D curve geometry using helper function
        Ptr<adsk::core::Curve3D> curve3D = getCurveWorldGeometry(sketchCurve);
        if (!curve3D) {
            logger->logWarning("Could not get geometry for curve " + std::to_string(i) + " using getCurveWorldGeometry");
            continue;  // Skip this curve
        }

        logger->logDebug("Successfully got geometry for curve " + std::to_string(i));

        // Get curve evaluator for tessellation
        logger->logDebug("About to get curve evaluator for curve " + std::to_string(i));

        Ptr<adsk::core::CurveEvaluator3D> evaluator;
        try {
            evaluator = curve3D->evaluator();
            if (!evaluator) {
                logger->logWarning("Could not get 3D curve evaluator for curve " + std::to_string(i));
                continue;
            }
            logger->logDebug("Got 3D curve evaluator for curve " + std::to_string(i));
        } catch (const std::exception& e) {
            logger->logError("Exception getting 3D curve evaluator for curve " + std::to_string(i) + ": " + std::string(e.what()));
            continue;
        } catch (...) {
            logger->logError("Unknown exception getting 3D curve evaluator for curve " + std::to_string(i));
            continue;
        }

        // Get parameter extents
        double startParam, endParam;
        if (!evaluator->getParameterExtents(startParam, endParam)) {
            logger->logWarning("Could not get parameter extents for curve " + std::to_string(i));
            continue;
        }

        logger->logDebug("Parameter extents for curve " + std::to_string(i) + ": " + std::to_string(startParam) + " to " + std::to_string(endParam));

        // Tessellate the curve using getStrokes (adaptive tessellation)
        std::vector<Ptr<adsk::core::Point3D>> strokePoints;
        if (!evaluator->getStrokes(startParam, endParam, chordTolerance, strokePoints)) {
            logger->logWarning("getStrokes failed for curve " + std::to_string(i));
            continue;
        }

        if (strokePoints.empty()) {
            logger->logWarning("getStrokes returned empty points for curve " + std::to_string(i));
            continue;
        }

        logger->logDebug("getStrokes succeeded for curve " + std::to_string(i) + ": " + std::to_string(strokePoints.size()) + " points");

        // Store stroke points and endpoints
        curveData.strokePoints = strokePoints;
        // Store start and end points for chaining
        curveData.startPoint = curveData.strokePoints.front();
        curveData.endPoint = curveData.strokePoints.back();

        // Log the endpoints for debugging
        logger->logDebug("Curve " + std::to_string(i) + " start: (" + std::to_string(curveData.startPoint->x()) + ", " + std::to_string(curveData.startPoint->y()) + ", " + std::to_string(curveData.startPoint->z()) + ")");
        logger->logDebug("Curve " + std::to_string(i) + " end: (" + std::to_string(curveData.endPoint->x()) + ", " + std::to_string(curveData.endPoint->y()) + ", " + std::to_string(curveData.endPoint->z()) + ")");

        allCurves.push_back(curveData);
    }

    logger->logDebug("Finished collecting all " + std::to_string(profileCurves->count()) + " curves");
    logger->logDebug("Found " + std::to_string(allCurves.size()) + " valid curves");

    if (allCurves.empty()) {
        logger->logError("No valid curves extracted from profile");
        return false;
    }

    return true;
}

}  // namespace Adapters
}  // namespace ChipCarving

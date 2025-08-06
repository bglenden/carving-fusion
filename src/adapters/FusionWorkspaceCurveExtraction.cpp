/**
 * FusionWorkspaceCurveExtraction.cpp
 *
 * Curve extraction and tessellation operations for FusionWorkspace
 * Split from FusionWorkspaceProfile.cpp for maintainability
 */

#include "../../include/utils/logging.h"
#include "FusionAPIAdapter.h"
#include "FusionWorkspaceProfileTypes.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

bool FusionWorkspace::extractCurvesFromProfile(adsk::core::Ptr<adsk::fusion::Profile> profile,
                                               std::vector<CurveData>& allCurves, TransformParams& transform) {
  LOG_DEBUG("Starting curve extraction from profile");

  if (!profile) {
    LOG_ERROR("Null profile provided for curve extraction");
    return false;
  }

  // Verify sketch plane orientation (should be parallel to XY plane)
  try {
    // Get the parent sketch from the profile
    Ptr<adsk::fusion::Sketch> profileSketch = profile->parentSketch();
    if (profileSketch) {
      LOG_DEBUG("Got parent sketch for profile");

      // Check if the sketch curves are planar
      // Fusion 360 sketches are always on a plane, but we can verify the curves
      // are 2D
      Ptr<adsk::fusion::SketchCurves> allSketchCurves = profileSketch->sketchCurves();
      if (allSketchCurves) {
        LOG_DEBUG("Sketch has " << allSketchCurves->count() << " curves");

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
                LOG_WARNING("Found 3D curve at index " << i);
              }
            } catch (...) {
              // Ignore property access errors
            }
          }
        }

        if (has3DCurves) {
          LOG_WARNING("Sketch contains 3D curves. Results may be unexpected.");
        } else {
          LOG_DEBUG("All checked curves appear to be 2D (planar)");
        }
      }

      // Get the sketch plane to verify orientation
      Ptr<adsk::core::Base> refPlane = profileSketch->referencePlane();
      if (refPlane) {
        LOG_DEBUG("Got reference plane for sketch");

        // Try to get the geometry of the plane
        // The reference plane could be a construction plane, datum plane, etc.
        Ptr<adsk::fusion::ConstructionPlane> constructionPlane = refPlane;
        if (constructionPlane) {
          Ptr<adsk::core::Plane> plane = constructionPlane->geometry();
          if (plane) {
            Ptr<adsk::core::Vector3D> normal = plane->normal();
            if (normal) {
              LOG_DEBUG("Sketch plane normal: (" << normal->x() << ", " << normal->y() << ", " << normal->z() << ")");

              // Check if the sketch plane is parallel to XY plane (normal close
              // to Z-axis)
              if (std::abs(normal->z()) > 0.99) {
                if (normal->z() > 0) {
                  LOG_DEBUG("Sketch plane is correctly oriented (parallel to XY "
                            "plane)");
                } else {
                  LOG_DEBUG("Sketch plane is parallel to XY plane but pointing down");
                }

                // Get the sketch plane Z position
                Ptr<adsk::core::Point3D> origin = plane->origin();
                if (origin) {
                  transform.sketchPlaneZ = origin->z();
                  LOG_DEBUG("Sketch plane Z position: " << transform.sketchPlaneZ << " cm");
                }
              }
            }
          }
        } else {
          LOG_DEBUG("Reference plane is not a construction plane");
        }
      }
    }
  } catch (const std::exception& e) {
    LOG_WARNING("Exception checking sketch plane: " << e.what());
  } catch (...) {
    LOG_WARNING("Unknown exception checking sketch plane");
  }

  // Get the loops from the profile (usually just one outer loop)
  Ptr<adsk::fusion::ProfileLoops> loops = profile->profileLoops();
  if (!loops || loops->count() == 0) {
    LOG_ERROR("No loops in profile");
    return false;
  }
  LOG_DEBUG("Found " << loops->count() << " loops");

  // Use the first loop (outer loop)
  Ptr<adsk::fusion::ProfileLoop> loop = loops->item(0);
  if (!loop) {
    LOG_ERROR("Could not get profile loop");
    return false;
  }

  // Get the curves in the loop
  Ptr<adsk::fusion::ProfileCurves> profileCurves = loop->profileCurves();
  if (!profileCurves || profileCurves->count() == 0) {
    LOG_ERROR("No curves in profile loop");
    return false;
  }
  LOG_DEBUG("Found " << profileCurves->count() << " curves in loop");

  // Check if the profile loop is closed
  LOG_DEBUG("Profile loop is " << (loop->isOuter() ? "closed (outer)" : "open (inner)"));

  // Tessellate each curve in the loop using getStrokes
  double chordTolerance = 0.01;  // 0.1mm tolerance (Fusion uses cm units)

  // CRITICAL FIX: Curves come in arbitrary order from Fusion, not connected
  // order We need to chain them properly to form a closed polygon
  LOG_DEBUG("Found " << profileCurves->count() << " curves - need to chain them in order");

  // First, collect all curves with their stroke points and endpoints
  allCurves.clear();
  allCurves.reserve(profileCurves->count());

  for (size_t i = 0; i < profileCurves->count(); ++i) {
    CurveData curveData;
    curveData.originalIndex = i;
    curveData.used = false;

    Ptr<adsk::fusion::ProfileCurve> profileCurve = profileCurves->item(i);
    if (!profileCurve) {
      LOG_WARNING("Null profile curve at index " << i);
      continue;
    }

    // Get the underlying sketch entity
    Ptr<adsk::fusion::SketchEntity> sketchEntity = profileCurve->sketchEntity();
    if (!sketchEntity) {
      LOG_WARNING("No sketch entity for curve " << i);
      continue;
    }

    // Cast to SketchCurve to get geometry
    Ptr<adsk::fusion::SketchCurve> sketchCurve = sketchEntity;
    if (!sketchCurve) {
      LOG_WARNING("Could not cast to SketchCurve for curve " << i);
      continue;
    }

    // Log detailed information about the curve type
    LOG_DEBUG("Curve " << i << " - SketchCurve type: " << sketchCurve->objectType());

    // Check what this object actually is and what methods it has
    if (Ptr<adsk::fusion::SketchLine> line = sketchCurve) {
      LOG_DEBUG("Curve " << i << " IS a SketchLine");
      // Try to get basic properties
      try {
        auto geom = line->geometry();
        LOG_DEBUG("SketchLine.geometry() returned: " << (geom ? "valid pointer" : "NULL"));
        if (geom) {
          LOG_DEBUG("Line geometry type: " << geom->objectType());
        }
      } catch (...) {
        LOG_ERROR("Exception calling SketchLine.geometry()");
      }
    } else {
      LOG_DEBUG("Curve " << i << " is NOT a SketchLine (casting failed)");
    }

    // Get the 3D curve geometry using helper function
    Ptr<adsk::core::Curve3D> curve3D = getCurveWorldGeometry(sketchCurve);
    if (!curve3D) {
      LOG_WARNING("Could not get geometry for curve " << i << " using getCurveWorldGeometry");
      continue;  // Skip this curve
    }

    LOG_DEBUG("Successfully got geometry for curve " << i);

    // Get curve evaluator for tessellation
    LOG_DEBUG("About to get curve evaluator for curve " << i);

    Ptr<adsk::core::CurveEvaluator3D> evaluator;
    try {
      evaluator = curve3D->evaluator();
      if (!evaluator) {
        LOG_WARNING("Could not get 3D curve evaluator for curve " << i);
        continue;
      }
      LOG_DEBUG("Got 3D curve evaluator for curve " << i);
    } catch (const std::exception& e) {
      LOG_ERROR("Exception getting 3D curve evaluator for curve " << i << ": " << e.what());
      continue;
    } catch (...) {
      LOG_ERROR("Unknown exception getting 3D curve evaluator for curve " << i);
      continue;
    }

    // Get parameter extents
    double startParam, endParam;
    if (!evaluator->getParameterExtents(startParam, endParam)) {
      LOG_WARNING("Could not get parameter extents for curve " << i);
      continue;
    }

    LOG_DEBUG("Parameter extents for curve " << i << ": " << startParam << " to " << endParam);

    // Tessellate the curve using getStrokes (adaptive tessellation)
    std::vector<Ptr<adsk::core::Point3D>> strokePoints;
    if (!evaluator->getStrokes(startParam, endParam, chordTolerance, strokePoints)) {
      LOG_WARNING("getStrokes failed for curve " << i);
      continue;
    }

    if (strokePoints.empty()) {
      LOG_WARNING("getStrokes returned empty points for curve " << i);
      continue;
    }

    LOG_DEBUG("getStrokes succeeded for curve " << i << ": " << strokePoints.size() << " points");

    // Store stroke points and endpoints
    curveData.strokePoints = strokePoints;
    // Store start and end points for chaining
    curveData.startPoint = curveData.strokePoints.front();
    curveData.endPoint = curveData.strokePoints.back();

    // Log the endpoints for debugging
    LOG_DEBUG("Curve " << i << " start: (" << curveData.startPoint->x() << ", " << curveData.startPoint->y() << ", "
                       << curveData.startPoint->z() << ")");
    LOG_DEBUG("Curve " << i << " end: (" << curveData.endPoint->x() << ", " << curveData.endPoint->y() << ", "
                       << curveData.endPoint->z() << ")");

    allCurves.push_back(curveData);
  }

  LOG_DEBUG("Finished collecting all " << profileCurves->count() << " curves");
  LOG_DEBUG("Found " << allCurves.size() << " valid curves");

  if (allCurves.empty()) {
    LOG_ERROR("No valid curves extracted from profile");
    return false;
  }

  return true;
}

}  // namespace Adapters
}  // namespace ChipCarving

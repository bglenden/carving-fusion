/**
 * PluginCommandsGeometryMain.cpp
 *
 * Main geometry extraction for PluginCommands
 * Part of PluginCommandsGeometry refactoring (Item #1a)
 * Extracted from PluginCommandsGeometry.cpp
 */

#include "../../include/utils/logging.h"
#include "PluginCommands.h"
#include "PluginCommandsGeometryChaining.h"

namespace ChipCarving {
namespace Commands {

void GeneratePathsCommandHandler::clearCachedGeometry() {
  cachedProfiles_.clear();
  LOG_INFO("Cleared cached profile geometry");
}

void GeneratePathsCommandHandler::extractAndCacheProfileGeometry(adsk::core::Ptr<adsk::fusion::Profile> profile,
                                                                 int index) {
  if (!profile) {
    LOG_INFO("Cannot extract geometry from null profile at index " << index);
    return;
  }

  try {
    ChipCarving::Adapters::ProfileGeometry profileGeom;

    // Get basic profile information while it's still valid
    auto sketch = profile->parentSketch();
    if (sketch) {
      profileGeom.sketchName = sketch->name();

      // Get plane entity ID
      auto referenceEntity = sketch->referencePlane();
      if (referenceEntity) {
        auto constructionPlane = referenceEntity->cast<adsk::fusion::ConstructionPlane>();
        if (constructionPlane) {
          profileGeom.planeEntityId = constructionPlane->entityToken();
        } else {
          auto face = referenceEntity->cast<adsk::fusion::BRepFace>();
          if (face) {
            profileGeom.planeEntityId = face->entityToken();
          }
        }
      }
    }

    // Extract area properties
    auto areaProps = profile->areaProperties();
    if (areaProps) {
      profileGeom.area = areaProps->area();
      auto centroid = areaProps->centroid();
      if (centroid) {
        profileGeom.centroid = {centroid->x(), centroid->y()};
      }
    }

    // CRITICAL: Extract all vertices immediately while profile is valid
    // We need to chain curves properly to avoid self-intersections

    std::vector<CurveData> allCurves;

    auto profileLoops = profile->profileLoops();
    if (profileLoops) {
      for (size_t loopIdx = 0; loopIdx < profileLoops->count(); ++loopIdx) {
        auto loop = profileLoops->item(static_cast<int>(loopIdx));
        if (!loop)
          continue;

        auto profileCurves = loop->profileCurves();
        if (!profileCurves)
          continue;

        // Collect curve data for proper chaining
        allCurves.reserve(profileCurves->count());
        for (size_t curveIdx = 0; curveIdx < profileCurves->count(); ++curveIdx) {
          CurveData curveData;
          curveData.originalIndex = curveIdx;
          curveData.used = false;

          auto profileCurve = profileCurves->item(static_cast<int>(curveIdx));
          if (!profileCurve)
            continue;

          auto sketchEntity = profileCurve->sketchEntity();
          if (!sketchEntity)
            continue;

          // Extract geometry using the same approach as existing code
          adsk::core::Ptr<adsk::core::Curve3D> worldGeometry = nullptr;

          // Get world geometry from sketch entity
          auto sketchCurve = sketchEntity->cast<adsk::fusion::SketchCurve>();
          if (sketchCurve) {
            // Try each entity type and get its world geometry
            if (auto line = sketchEntity->cast<adsk::fusion::SketchLine>()) {
              worldGeometry = line->worldGeometry();
              LOG_INFO("    Curve " << curveIdx << " is a SketchLine");
            } else if (auto arc = sketchEntity->cast<adsk::fusion::SketchArc>()) {
              worldGeometry = arc->worldGeometry();
              LOG_INFO("    Curve " << curveIdx << " is a SketchArc");
            } else if (auto circle = sketchEntity->cast<adsk::fusion::SketchCircle>()) {
              worldGeometry = circle->worldGeometry();
              LOG_INFO("    Curve " << curveIdx << " is a SketchCircle");
            } else if (auto spline = sketchEntity->cast<adsk::fusion::SketchFittedSpline>()) {
              worldGeometry = spline->worldGeometry();
              LOG_INFO("    Curve " << curveIdx << " is a SketchFittedSpline");
            } else if (auto nurbs = sketchEntity->cast<adsk::fusion::SketchControlPointSpline>()) {
              worldGeometry = nurbs->worldGeometry();
              LOG_INFO("    Curve " << curveIdx << " is a SketchControlPointSpline");
            } else if (auto ellipse = sketchEntity->cast<adsk::fusion::SketchEllipse>()) {
              worldGeometry = ellipse->worldGeometry();
              LOG_INFO("    Curve " << curveIdx << " is a SketchEllipse");
            } else if (auto ellipticalArc = sketchEntity->cast<adsk::fusion::SketchEllipticalArc>()) {
              worldGeometry = ellipticalArc->worldGeometry();
              LOG_INFO("    Curve " << curveIdx << " is a SketchEllipticalArc");
            }
          }

          if (worldGeometry) {
            // Use tessellation like existing code to get proper polygon
            // vertices
            auto evaluator = worldGeometry->evaluator();
            if (evaluator) {
              double startParam, endParam;
              if (evaluator->getParameterExtents(startParam, endParam)) {
                // Use getStrokes for proper tessellation
                std::vector<adsk::core::Ptr<adsk::core::Point3D>> strokePoints;

                // Determine tolerance based on curve type
                double chordTolerance = 0.01;  // Default: 0.1mm tolerance for accurate curves

                // For lines, use coarser tolerance since they don't need
                // tessellation
                if (auto line = sketchEntity->cast<adsk::fusion::SketchLine>()) {
                  chordTolerance = 0.1;  // 1mm tolerance for lines (should only
                                         // give 2 points)
                } else {
                  // For arcs, circles, splines etc., use fine tolerance for
                  // smooth curves
                  chordTolerance = 0.005;  // 0.05mm tolerance for accurate
                                           // curve representation
                }

                LOG_INFO("    Using tolerance " << chordTolerance << " cm for tessellation");
                if (evaluator->getStrokes(startParam, endParam, chordTolerance, strokePoints)) {
                  LOG_INFO("    Generated " << strokePoints.size() << " stroke points");

                  // Validate tessellation quality for non-linear curves
                  if (!sketchEntity->cast<adsk::fusion::SketchLine>() && strokePoints.size() < 3) {
                    LOG_WARNING("    Insufficient tessellation for curve " + std::to_string(curveIdx) + " (" +
                                std::to_string(strokePoints.size()) + " points) - attempting finer tolerance");

                    // Try again with finer tolerance
                    strokePoints.clear();
                    chordTolerance = 0.001;  // 0.01mm for very fine tessellation
                    if (evaluator->getStrokes(startParam, endParam, chordTolerance, strokePoints)) {
                      LOG_INFO("    Retessellated with finer tolerance: " << strokePoints.size() << " points");
                    }
                  }
                  // Store stroke points and endpoints for chaining
                  curveData.strokePoints = strokePoints;
                  if (!strokePoints.empty()) {
                    curveData.startPoint = strokePoints.front();
                    curveData.endPoint = strokePoints.back();
                  }
                  allCurves.push_back(curveData);
                } else {
                  LOG_ERROR("    getStrokes failed for curve " << curveIdx
                                                               << " - geometry will be missing from profile");
                  // Try fallback with endpoints only for critical path
                  // continuity
                  std::vector<adsk::core::Ptr<adsk::core::Point3D>> fallbackPoints;
                  adsk::core::Ptr<adsk::core::Point3D> startPt, endPt;
                  if (evaluator->getPointAtParameter(startParam, startPt) &&
                      evaluator->getPointAtParameter(endParam, endPt)) {
                    fallbackPoints.push_back(startPt);
                    fallbackPoints.push_back(endPt);
                    curveData.strokePoints = fallbackPoints;
                    curveData.startPoint = startPt;
                    curveData.endPoint = endPt;
                    allCurves.push_back(curveData);
                    LOG_WARNING("    Using fallback endpoints-only approach for "
                                "curve " +
                                std::to_string(curveIdx));
                  }
                }
              }
            }
          }
        }
      }
    }

    // Delegate to chaining logic in separate file
    profileGeom.vertices = chainCurvesAndExtractVertices(allCurves);

    // Validate extraction results
    if (profileGeom.vertices.size() < 3) {
      LOG_ERROR("Extracted polygon has insufficient vertices (" << profileGeom.vertices.size()
                                                                << ") - minimum 3 required for valid polygon");
    }

    // Set transform parameters (identity for now since vertices are in world
    // coordinates)
    profileGeom.transform.centerX = 0.0;
    profileGeom.transform.centerY = 0.0;
    profileGeom.transform.scale = 1.0;

    // Log detailed geometry information for debugging
    LOG_INFO("Extracted " << profileGeom.vertices.size() << " vertices from profile " << index);
    if (!profileGeom.vertices.empty()) {
      // Log first few vertices for debugging
      size_t numToLog = std::min(size_t(6), profileGeom.vertices.size());
      for (size_t i = 0; i < numToLog; ++i) {
        LOG_INFO("  Vertex " << i << ": (" << profileGeom.vertices[i].first << ", " << profileGeom.vertices[i].second
                             << ")");
      }
      if (profileGeom.vertices.size() > 6) {
        LOG_INFO("  ... and " << (profileGeom.vertices.size() - 6) << " more vertices");
      }

      // Calculate and log bounding box
      double minX = profileGeom.vertices[0].first, maxX = profileGeom.vertices[0].first;
      double minY = profileGeom.vertices[0].second, maxY = profileGeom.vertices[0].second;
      for (const auto& v : profileGeom.vertices) {
        minX = std::min(minX, v.first);
        maxX = std::max(maxX, v.first);
        minY = std::min(minY, v.second);
        maxY = std::max(maxY, v.second);
      }
      LOG_INFO("  Bounding box: (" << minX << ", " << minY << ") to (" << maxX << ", " << maxY << ")");
      LOG_INFO("  Size: " << (maxX - minX) << " x " << (maxY - minY) << " cm");
    }

    // Store in cache
    if (index >= static_cast<int>(cachedProfiles_.size())) {
      cachedProfiles_.resize(index + 1);
    }
    cachedProfiles_[index] = profileGeom;

    LOG_INFO("Successfully cached geometry for profile " << index << " from sketch '" << profileGeom.sketchName
                                                         << "' with " << profileGeom.vertices.size()
                                                         << " vertices and area " << profileGeom.area << " sq cm");
  } catch (const std::exception& e) {
    LOG_INFO("Exception during geometry extraction for profile " << index << ": " << e.what());
  } catch (...) {
    LOG_INFO("Unknown exception during geometry extraction for profile " << index);
  }
}

}  // namespace Commands
}  // namespace ChipCarving

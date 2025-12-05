/**
 * FusionWorkspaceProfileGeometry.cpp
 *
 * Profile geometry extraction operations for FusionWorkspace
 * Split from FusionWorkspaceProfile.cpp for maintainability
 */

#include <cmath>

#include "FusionAPIAdapter.h"
#include "FusionWorkspaceProfileTypes.h"
#include "utils/logging.h"

using adsk::core::Ptr;

namespace ChipCarving {
namespace Adapters {

bool FusionWorkspace::extractProfileGeometry(Ptr<adsk::fusion::Profile> profile, ProfileGeometry& geometry) {
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
  Ptr<adsk::fusion::ProfileLoops> profileLoops = profile->profileLoops();
  if (!profileLoops || profileLoops->count() == 0) {
    LOG_ERROR("Profile has no loops");
    return false;
  }

  Ptr<adsk::fusion::ProfileLoop> firstLoop = profileLoops->item(0);
  if (!firstLoop || !firstLoop->isValid()) {
    LOG_ERROR("Profile has no valid loops");
    return false;
  }

  Ptr<adsk::fusion::ProfileCurves> loopCurves = firstLoop->profileCurves();
  if (!loopCurves || loopCurves->count() == 0) {
    LOG_ERROR("Profile loop has no curves");
    return false;
  }

  Ptr<adsk::fusion::ProfileCurve> firstCurve = loopCurves->item(0);
  if (!firstCurve || !firstCurve->isValid() || !firstCurve->sketchEntity()) {
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
      if (allCurves[i].used)
        continue;

      double distToStart = std::sqrt(std::pow(allCurves[i].startPoint->x() - currentEndPoint->x(), 2) +
                                     std::pow(allCurves[i].startPoint->y() - currentEndPoint->y(), 2));

      double distToEnd = std::sqrt(std::pow(allCurves[i].endPoint->x() - currentEndPoint->x(), 2) +
                                   std::pow(allCurves[i].endPoint->y() - currentEndPoint->y(), 2));

      if (distToStart < tolerance) {
        chainOrder.push_back(i);
        allCurves[i].used = true;
        currentEndPoint = allCurves[i].endPoint;
        foundNext = true;
        break;
      }
      if (distToEnd < tolerance) {
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

  LOG_DEBUG("Extracted ProfileGeometry with " << geometry.vertices.size() << " vertices, area=" << geometry.area
                                              << " sq cm");

  return true;
}

}  // namespace Adapters
}  // namespace ChipCarving

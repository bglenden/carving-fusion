/**
 * FusionWorkspaceCurveGeometry.cpp
 *
 * Curve geometry extraction operations for FusionWorkspace
 * Split from FusionWorkspaceCurve.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "FusionAPIAdapter.h"
#include "../../include/utils/TempFileManager.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

Ptr<adsk::core::Curve3D> FusionWorkspace::getCurveWorldGeometry(
    Ptr<adsk::fusion::SketchCurve> sketchCurve) {
    if (!sketchCurve) {
        return nullptr;
    }

    std::string curveType = sketchCurve->objectType();

    // FIXED: Use worldGeometry() to get WORLD coordinates for medial axis computation
    // The medial axis processor expects world coordinates, not local sketch coordinates
    if (curveType == "adsk::fusion::SketchLine") {
        Ptr<adsk::fusion::SketchLine> line = sketchCurve;
        if (line) {
            return line->worldGeometry();
        }
    } else if (curveType == "adsk::fusion::SketchArc") {
        Ptr<adsk::fusion::SketchArc> arc = sketchCurve;
        if (arc) {
            return arc->worldGeometry();
        }
    } else if (curveType == "adsk::fusion::SketchCircle") {
        Ptr<adsk::fusion::SketchCircle> circle = sketchCurve;
        if (circle) {
            return circle->worldGeometry();
        }
    } else if (curveType == "adsk::fusion::SketchFittedSpline") {
        Ptr<adsk::fusion::SketchFittedSpline> spline = sketchCurve;
        if (spline) {
            return spline->worldGeometry();
        }
    } else if (curveType == "adsk::fusion::SketchFixedSpline") {
        Ptr<adsk::fusion::SketchFixedSpline> spline = sketchCurve;
        if (spline) {
            return spline->worldGeometry();
        }
    } else if (curveType == "adsk::fusion::SketchEllipse") {
        Ptr<adsk::fusion::SketchEllipse> ellipse = sketchCurve;
        if (ellipse) {
            return ellipse->worldGeometry();
        }
    } else if (curveType == "adsk::fusion::SketchEllipticalArc") {
        Ptr<adsk::fusion::SketchEllipticalArc> ellipticalArc = sketchCurve;
        if (ellipticalArc) {
            return ellipticalArc->worldGeometry();
        }
    } else if (curveType == "adsk::fusion::SketchConicCurve") {
        Ptr<adsk::fusion::SketchConicCurve> conic = sketchCurve;
        if (conic) {
            return conic->worldGeometry();
        }
    }

    // Unknown curve type
    return nullptr;
}

}  // namespace Adapters
}  // namespace ChipCarving

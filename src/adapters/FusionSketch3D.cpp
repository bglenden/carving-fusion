/**
 * FusionSketch3D.cpp
 *
 * 3D geometry functionality for FusionSketch
 * Split from FusionSketch.cpp for maintainability
 */

#include <cmath>
#include <iostream>

#include "../../include/geometry/Shape.h"
#include "../../include/geometry/Point3D.h"
#include "FusionAPIAdapter.h"
#include "../utils/UnitConversion.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

bool FusionSketch::addSpline3D(const std::vector<Geometry::Point3D>& points) {
    if (!sketch_ || points.size() < 2) {
        return false;
    }

    try {
        // Create 3D points for the spline
        adsk::core::Ptr<adsk::core::ObjectCollection> point3DCollection =
            adsk::core::ObjectCollection::create();

        for (const auto& point : points) {
            // Convert from mm to cm (Fusion's internal units)
            double x_cm = point.x / 10.0;
            double y_cm = point.y / 10.0;
            double z_cm = point.z / 10.0;

            adsk::core::Ptr<adsk::core::Point3D> fusionPoint =
                adsk::core::Point3D::create(x_cm, y_cm, z_cm);
            if (fusionPoint) {
                point3DCollection->add(fusionPoint);
            }
        }

        if (point3DCollection->count() < 2) {
            return false;
        }

        // Create the 3D spline curve
        adsk::core::Ptr<adsk::fusion::SketchFittedSplines> splines = sketch_->sketchCurves()->sketchFittedSplines();
        if (splines) {
            adsk::core::Ptr<adsk::fusion::SketchFittedSpline> spline =
                splines->add(point3DCollection);
            return spline != nullptr;
        }

        return false;

    } catch (...) {
        return false;
    }
}

bool FusionSketch::addLine3D(double x1, double y1, double z1, double x2, double y2, double z2) {
    if (!sketch_) {
        return false;
    }

    try {
        // Convert from mm to cm (Fusion's internal units)
        double x1_cm = x1 / 10.0;
        double y1_cm = y1 / 10.0;
        double z1_cm = z1 / 10.0;
        double x2_cm = x2 / 10.0;
        double y2_cm = y2 / 10.0;
        double z2_cm = z2 / 10.0;

        // Create 3D points
        adsk::core::Ptr<adsk::core::Point3D> startPoint =
            adsk::core::Point3D::create(x1_cm, y1_cm, z1_cm);
        adsk::core::Ptr<adsk::core::Point3D> endPoint =
            adsk::core::Point3D::create(x2_cm, y2_cm, z2_cm);

        if (!startPoint || !endPoint) {
            return false;
        }

        // Create the 3D line
        adsk::core::Ptr<adsk::fusion::SketchLines> lines = sketch_->sketchCurves()->sketchLines();
        if (lines) {
            adsk::core::Ptr<adsk::fusion::SketchLine> line =
                lines->addByTwoPoints(startPoint, endPoint);
            return line != nullptr;
        }

        return false;

    } catch (...) {
        return false;
    }
}

bool FusionSketch::addPoint3D(double x, double y, double z) {
    if (!sketch_) {
        return false;
    }

    try {
        // Convert from mm to cm (Fusion's internal units)
        double x_cm = x / 10.0;
        double y_cm = y / 10.0;
        double z_cm = z / 10.0;

        // Create 3D point
        adsk::core::Ptr<adsk::core::Point3D> point3D =
            adsk::core::Point3D::create(x_cm, y_cm, z_cm);

        if (!point3D) {
            return false;
        }

        // Add the 3D point to the sketch
        adsk::core::Ptr<adsk::fusion::SketchPoints> points = sketch_->sketchPoints();
        if (points) {
            adsk::core::Ptr<adsk::fusion::SketchPoint> sketchPoint =
                points->add(point3D);
            return sketchPoint != nullptr;
        }

        return false;

    } catch (...) {
        return false;
    }
}

std::vector<std::string> FusionSketch::getSketchCurveEntityIds() {
    std::vector<std::string> entityIds;

    if (!sketch_) {
        return entityIds;
    }

    try {
        // Get all sketch curves collection
        adsk::core::Ptr<adsk::fusion::SketchCurves> sketchCurves = sketch_->sketchCurves();
        if (!sketchCurves) {
            return entityIds;
        }

        // Get all lines
        adsk::core::Ptr<adsk::fusion::SketchLines> lines = sketchCurves->sketchLines();
        if (lines) {
            for (size_t i = 0; i < lines->count(); ++i) {
                adsk::core::Ptr<adsk::fusion::SketchLine> line = lines->item(static_cast<int>(i));
                if (line && line->entityToken().length() > 0) {
                    entityIds.push_back(line->entityToken());
                }
            }
        }

        // Get all arcs
        adsk::core::Ptr<adsk::fusion::SketchArcs> arcs = sketchCurves->sketchArcs();
        if (arcs) {
            for (size_t i = 0; i < arcs->count(); ++i) {
                adsk::core::Ptr<adsk::fusion::SketchArc> arc = arcs->item(static_cast<int>(i));
                if (arc && arc->entityToken().length() > 0) {
                    entityIds.push_back(arc->entityToken());
                }
            }
        }

        // Get all splines
        adsk::core::Ptr<adsk::fusion::SketchFittedSplines> splines = sketchCurves->sketchFittedSplines();
        if (splines) {
            for (size_t i = 0; i < splines->count(); ++i) {
                adsk::core::Ptr<adsk::fusion::SketchFittedSpline> spline = splines->item(static_cast<int>(i));
                if (spline && spline->entityToken().length() > 0) {
                    entityIds.push_back(spline->entityToken());
                }
            }
        }

        // Get all circles
        adsk::core::Ptr<adsk::fusion::SketchCircles> circles = sketchCurves->sketchCircles();
        if (circles) {
            for (size_t i = 0; i < circles->count(); ++i) {
                adsk::core::Ptr<adsk::fusion::SketchCircle> circle = circles->item(static_cast<int>(i));
                if (circle && circle->entityToken().length() > 0) {
                    entityIds.push_back(circle->entityToken());
                }
            }
        }

        // Get all ellipses
        adsk::core::Ptr<adsk::fusion::SketchEllipses> ellipses = sketchCurves->sketchEllipses();
        if (ellipses) {
            for (size_t i = 0; i < ellipses->count(); ++i) {
                adsk::core::Ptr<adsk::fusion::SketchEllipse> ellipse = ellipses->item(static_cast<int>(i));
                if (ellipse && ellipse->entityToken().length() > 0) {
                    entityIds.push_back(ellipse->entityToken());
                }
            }
        }

        // Get all elliptical arcs
        adsk::core::Ptr<adsk::fusion::SketchEllipticalArcs> ellipticalArcs = sketchCurves->sketchEllipticalArcs();
        if (ellipticalArcs) {
            for (size_t i = 0; i < ellipticalArcs->count(); ++i) {
                adsk::core::Ptr<adsk::fusion::SketchEllipticalArc> ellipticalArc = ellipticalArcs->item(static_cast<int>(i));
                if (ellipticalArc && ellipticalArc->entityToken().length() > 0) {
                    entityIds.push_back(ellipticalArc->entityToken());
                }
            }
        }

    } catch (const std::exception& e) {
        std::cout << "Error getting sketch curve entity IDs: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown error getting sketch curve entity IDs" << std::endl;
    }

    return entityIds;
}

}  // namespace Adapters
}  // namespace ChipCarving

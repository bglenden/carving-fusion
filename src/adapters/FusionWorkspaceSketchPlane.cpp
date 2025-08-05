/**
 * FusionWorkspaceSketchPlane.cpp
 *
 * Plane-based sketch creation and finding operations for FusionWorkspace
 * Split from FusionWorkspaceSketch.cpp for maintainability
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#include "FusionAPIAdapter.h"
#include "../../include/utils/logging.h"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

std::unique_ptr<ISketch> FusionWorkspace::createSketchOnPlane(const std::string& name,
                                                               const std::string& planeEntityId) {
    if (!app_) {
        return nullptr;
    }

    try {
        // Get the active design
        Ptr<adsk::fusion::Design> design = app_->activeProduct();
        if (!design) {
            return nullptr;
        }

        // Get the root component
        Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
        if (!rootComp) {
            return nullptr;
        }

        // Get the sketches collection
        Ptr<adsk::fusion::Sketches> sketches = rootComp->sketches();
        if (!sketches) {
            return nullptr;
        }

        // Find the plane/surface using the entity ID
        Ptr<adsk::core::Base> planeEntity = nullptr;

        if (planeEntityId.empty()) {
            // No plane specified, use XY plane
            planeEntity = rootComp->xYConstructionPlane();
        } else {
            // Log the search for entity token
            LOG_DEBUG("Searching for plane entity with token: " << planeEntityId);

            // Try to find construction plane first
            Ptr<adsk::fusion::ConstructionPlanes> constructionPlanes = rootComp->constructionPlanes();
            if (constructionPlanes) {
                for (size_t i = 0; i < constructionPlanes->count(); ++i) {
                    Ptr<adsk::fusion::ConstructionPlane> plane = constructionPlanes->item(i);
                    if (plane && plane->entityToken() == planeEntityId) {
                        planeEntity = plane;
                        break;
                    }
                }
            }

            // If not found, try to find as a face
            if (!planeEntity) {
                Ptr<adsk::fusion::BRepBodies> bodies = rootComp->bRepBodies();
                if (bodies) {
                    for (size_t i = 0; i < bodies->count() && !planeEntity; ++i) {
                        Ptr<adsk::fusion::BRepBody> body = bodies->item(i);
                        if (!body) continue;

                        Ptr<adsk::fusion::BRepFaces> faces = body->faces();
                        if (!faces) continue;

                        for (size_t j = 0; j < faces->count(); ++j) {
                            Ptr<adsk::fusion::BRepFace> face = faces->item(j);
                            if (face && face->entityToken() == planeEntityId) {
                                planeEntity = face;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!planeEntity) {
            // Fall back to XY plane if entity not found
            LOG_DEBUG("Failed to resolve plane entity token: " << planeEntityId << ". Falling back to XY plane.");
            planeEntity = rootComp->xYConstructionPlane();
        } else {
            // Log successful entity token resolution
            LOG_DEBUG("Successfully resolved plane entity token: " << planeEntityId);
        }

        // Validate that the plane is parallel to XY
        bool isValidPlane = false;
        double planeZ = 0.0;  // TODO(developer): Use planeZ for offset sketch creation in future versions
        (void)planeZ;  // Suppress unused variable warning

        // Check if it's a construction plane
        Ptr<adsk::fusion::ConstructionPlane> constructionPlane = planeEntity;
        if (constructionPlane) {
            Ptr<adsk::core::Plane> geometry = constructionPlane->geometry();
            if (geometry) {
                Ptr<adsk::core::Vector3D> normal = geometry->normal();
                if (normal) {
                    // Check if normal is parallel to Z axis (0, 0, ±1)
                    double tolerance = 0.001;
                    if (std::abs(normal->x()) < tolerance &&
                        std::abs(normal->y()) < tolerance &&
                        std::abs(std::abs(normal->z()) - 1.0) < tolerance) {
                        isValidPlane = true;
                        Ptr<adsk::core::Point3D> origin = geometry->origin();
                        if (origin) {
                            planeZ = origin->z();
                        }
                    }
                }
            }
        } else {
            // Check if it's a planar face
            Ptr<adsk::fusion::BRepFace> face = planeEntity;
            if (face) {
                Ptr<adsk::core::Surface> surface = face->geometry();
                if (surface) {
                    // Check if it's a plane
                    Ptr<adsk::core::Plane> plane = surface;
                    if (plane) {
                        Ptr<adsk::core::Vector3D> normal = plane->normal();
                        if (normal) {
                            // Check if normal is parallel to Z axis
                            double tolerance = 0.001;
                            if (std::abs(normal->x()) < tolerance &&
                                std::abs(normal->y()) < tolerance &&
                                std::abs(std::abs(normal->z()) - 1.0) < tolerance) {
                                isValidPlane = true;
                                Ptr<adsk::core::Point3D> origin = plane->origin();
                                if (origin) {
                                    planeZ = origin->z();
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!isValidPlane && !planeEntityId.empty()) {
            // Log error and fall back to XY plane
            LOG_ERROR("Selected plane/surface is not parallel to XY plane. Using XY plane instead.");
            planeEntity = rootComp->xYConstructionPlane();
        }

        // Create the sketch on the plane
        Ptr<adsk::fusion::Sketch> sketch = sketches->add(planeEntity);
        if (!sketch) {
            return nullptr;
        }

        // Set the sketch name
        sketch->name(name);

        return std::make_unique<FusionSketch>(name, app_, sketch);

    } catch (const std::exception& e) {
        std::cout << "Sketch creation on plane error: " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cout << "Unknown sketch creation on plane error" << std::endl;
        return nullptr;
    }
}

std::unique_ptr<ISketch> FusionWorkspace::findSketch(const std::string& name) {
    if (!app_) {
        return nullptr;
    }

    try {
        // Get the active design
        Ptr<adsk::fusion::Design> design = app_->activeProduct();
        if (!design) {
            return nullptr;
        }

        // Get the root component
        Ptr<adsk::fusion::Component> rootComp = design->rootComponent();
        if (!rootComp) {
            return nullptr;
        }

        // Get all sketches
        Ptr<adsk::fusion::Sketches> sketches = rootComp->sketches();
        if (!sketches) {
            return nullptr;
        }

        // Search for sketch by name
        for (size_t i = 0; i < sketches->count(); ++i) {
            Ptr<adsk::fusion::Sketch> sketch = sketches->item(i);
            if (sketch && sketch->name() == name) {
                // Found existing sketch with matching name
                return std::make_unique<FusionSketch>(name, app_, sketch);
            }
        }

        // No sketch found with this name
        return nullptr;

    } catch (const std::exception& e) {
        std::cout << "Find sketch error: " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cout << "Unknown find sketch error" << std::endl;
        return nullptr;
    }
}

}  // namespace Adapters
}  // namespace ChipCarving

/**
 * PluginCommandsParameters.cpp
 *
 * Parameter input creation and processing for PluginCommands
 * Split from PluginCommands.cpp for maintainability
 */

#include "PluginCommands.h"
#include "../utils/UnitConversion.h"

using namespace ChipCarving::Utils;

namespace ChipCarving {
namespace Commands {

void GeneratePathsCommandHandler::createParameterInputs(
    adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
    try {
        // Add wide description to make dialog wider
        adsk::core::Ptr<adsk::core::TextBoxCommandInput> titleDesc =
            inputs->addTextBoxCommandInput("titleDescription", "",
                "<b>Generate V-Carve Toolpaths</b><br/>"
                "Select closed sketch profiles and configure tool parameters to generate medial axis construction geometry and V-carve toolpaths for CNC machining of chip carving patterns.",
                3, true);


        // 1. SHAPES TO CARVE (always expanded, no arrow)
        adsk::core::Ptr<adsk::core::GroupCommandInput> selectionGroup =
            inputs->addGroupCommandInput("selectionGroup", "Shapes to Carve");
        selectionGroup->isExpanded(true);
        selectionGroup->isEnabledCheckBoxDisplayed(false);
        adsk::core::Ptr<adsk::core::CommandInputs> selectionInputs = selectionGroup->children();

        // Selection input for sketch profiles (closed paths) with detailed tooltip
        adsk::core::Ptr<adsk::core::SelectionCommandInput> sketchSelection =
            selectionInputs->addSelectionInput("sketchProfiles", "Closed Sketch Profiles",
                                           "Select closed sketch profiles");
        // CRITICAL FIX: Use "Profiles" filter - this automatically ensures only closed profiles
        sketchSelection->addSelectionFilter("Profiles");
        sketchSelection->setSelectionLimits(1, 0);  // At least 1, no upper limit

        // Set detailed tooltip with enhanced selection instructions
        sketchSelection->tooltip(
            "Select closed sketch profiles ONLY (\"Profiles\" filter active).\n\n"
            "• ONLY closed sketch profiles (blue shaded regions) are selectable\n"
            "• Individual curves, lines, or arcs CANNOT be selected with this filter\n"
            "• Click INSIDE the blue shaded areas to select complete profiles\n"
            "• The \"Profiles\" filter automatically prevents invalid selections\n"
            "• Use Ctrl+Click to select multiple closed profiles");

        // 2. V-CARVE TOOLPATHS (always expanded, no arrow)
        adsk::core::Ptr<adsk::core::GroupCommandInput> vcarveGroup =
            inputs->addGroupCommandInput("vcarveGroup", "V-Carve Toolpaths");
        vcarveGroup->isExpanded(true);
        vcarveGroup->isEnabledCheckBoxDisplayed(false);
        adsk::core::Ptr<adsk::core::CommandInputs> vcarveInputs = vcarveGroup->children();

        // Tool selection dropdown
        adsk::core::Ptr<adsk::core::DropDownCommandInput> toolDropdown =
            vcarveInputs->addDropDownCommandInput("toolSelection", "Tool",
                                           adsk::core::DropDownStyles::TextListDropDownStyle);
        toolDropdown->listItems()->add("30° V-bit", false);
        toolDropdown->listItems()->add("60° V-bit", false);
        toolDropdown->listItems()->add("90° V-bit", true);  // Default selection
        toolDropdown->tooltip("Select the V-bit tool for path generation");

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> generateVCarve =
            vcarveInputs->addBoolValueInput("generateVCarveToolpaths", "Generate V-Carve Toolpaths",
                                           true, "", true);
        generateVCarve->tooltip("Generate 3D V-carve toolpaths from medial axis data");

        // V-carve safety parameters - FIXED UNITS
        adsk::core::Ptr<adsk::core::ValueCommandInput> maxDepth =
            vcarveInputs->addValueInput("maxVCarveDepth", "Maximum Depth", "mm",
                                       adsk::core::ValueInput::createByReal(2.5));
        maxDepth->tooltip("Maximum allowed V-carve depth for safety (default: 25.0mm)");

        // Always enable project to surface - it's the only mode we use
        adsk::core::Ptr<adsk::core::SelectionCommandInput> surfaceSelection =
            vcarveInputs->addSelectionInput("targetSurface", "Target Surface", "Select surface for projection");
        surfaceSelection->addSelectionFilter("Faces");  // Only allow face/surface selection
        // Note: Selection limits not available in API, user can select multiple but we only use first
        surfaceSelection->tooltip("Select a surface to project the V-carve toolpaths onto");

        // 3. VISUALIZATION OPTIONS (collapsible, default closed)
        adsk::core::Ptr<adsk::core::GroupCommandInput> constructionGroup =
            inputs->addGroupCommandInput("constructionGroup", "Visualization Options");
        constructionGroup->isExpanded(false);
        constructionGroup->isEnabledCheckBoxDisplayed(false);
        adsk::core::Ptr<adsk::core::CommandInputs> constructionInputs =
            constructionGroup->children();

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> generateViz =
            constructionInputs->addBoolValueInput("generateVisualization", "Generate Visualization",
                                                  true, "", false);
        generateViz->tooltip("Generate visualization sketches (default: off)");

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> showMedial =
            constructionInputs->addBoolValueInput("showMedialLines", "Medial Axis Lines", true, "",
                                                  true);
        showMedial->tooltip("Display medial axis centerlines as construction geometry");

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> showClearance =
            constructionInputs->addBoolValueInput("showClearanceCircles", "Tool Clearance Circles",
                                                  true, "", true);
        showClearance->tooltip("Display tool clearance circles at key points along medial axis");

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> showPolygon =
            constructionInputs->addBoolValueInput("showPolygonizedShape", "Polygonized Boundaries",
                                                  true, "", false);
        showPolygon->tooltip("Display the polygon approximation used for medial axis computation");

        // Cross size - FIXED UNITS (default 0.0mm = no crosses)
        adsk::core::Ptr<adsk::core::ValueCommandInput> crossSize =
            constructionInputs->addValueInput("crossSize", "Center Cross Size", "mm",
                                              adsk::core::ValueInput::createByReal(0.0));
        crossSize->tooltip(
            "Size of cross marks at clearance circle centers in mm (0 = no crosses, default: 0.0mm)");

        // 4. MEDIAL AXIS PARAMETERS (collapsible, default closed)
        adsk::core::Ptr<adsk::core::GroupCommandInput> paramGroup =
            inputs->addGroupCommandInput("paramGroup", "Medial Axis Parameters");
        paramGroup->isExpanded(false);
        paramGroup->isEnabledCheckBoxDisplayed(false);
        adsk::core::Ptr<adsk::core::CommandInputs> groupInputs = paramGroup->children();

        // Polygon tolerance (max allowed error when approximating curves) - FIXED UNITS
        // Fusion 360 internal units are cm, so 0.25mm = 0.025cm
        adsk::core::Ptr<adsk::core::ValueCommandInput> polygonTol =
            groupInputs->addValueInput("polygonTolerance", "Polygon Error Tolerance", "mm",
                                       adsk::core::ValueInput::createByReal(0.025));
        polygonTol->tooltip("Maximum allowed error when approximating curved edges with line "
                            "segments (default: 0.25mm)");

        // Force boundary intersections
        adsk::core::Ptr<adsk::core::BoolValueCommandInput> forceBoundary =
            groupInputs->addBoolValueInput("forceBoundaryIntersections",
                                           "Force Boundary Intersections", true, "", true);
        forceBoundary->tooltip(
            "Ensure every intersection with shape boundary is included in the path");

        // Surface sampling distance - controls V-carve path density for better surface following
        // Fusion 360 internal units are cm, so 2.0mm = 0.2cm
        adsk::core::Ptr<adsk::core::ValueCommandInput> samplingDistance =
            groupInputs->addValueInput("surfaceSamplingDistance", "Surface Sampling Distance", "mm",
                                       adsk::core::ValueInput::createByReal(0.2));
        samplingDistance->tooltip("Distance between V-carve points for surface following (smaller = more accurate, default: 2.0mm)");

    } catch (...) {
        // Ignore input creation errors for now
    }
}

ChipCarving::Adapters::MedialAxisParameters GeneratePathsCommandHandler::getParametersFromInputs(
    adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
    ChipCarving::Adapters::MedialAxisParameters params;

    try {
        // Get tool selection
        adsk::core::Ptr<adsk::core::DropDownCommandInput> toolDropdown =
            inputs->itemById("toolSelection");
        if (toolDropdown && toolDropdown->selectedItem()) {
            params.toolName = toolDropdown->selectedItem()->name();
            // Set tool angle based on selection
            if (params.toolName == "30° V-bit") {
                params.toolAngle = 30.0;
            } else if (params.toolName == "60° V-bit") {
                params.toolAngle = 60.0;
            } else if (params.toolName == "90° V-bit") {
                params.toolAngle = 90.0;
            }
        }

        adsk::core::Ptr<adsk::core::ValueCommandInput> polygonTol =
            inputs->itemById("polygonTolerance");
        if (polygonTol) {
            // Convert from Fusion's database units (cm) to mm
            params.polygonTolerance = fusionLengthToMm(polygonTol->value());
        }

        // Read surface sampling distance from dialog input
        adsk::core::Ptr<adsk::core::ValueCommandInput> samplingDistanceInput =
            inputs->itemById("surfaceSamplingDistance");
        if (samplingDistanceInput) {
            // Convert from Fusion's database units (cm) to mm
            params.samplingDistance = fusionLengthToMm(samplingDistanceInput->value());
        } else {
            // Fallback to default if input not found
            params.samplingDistance = 2.0;  // 2mm default
        }

        // REMOVED: Reading clearanceCircleSpacing - no longer needed
        // Set default clearance circle spacing (not used, but may be expected by other code)
        params.clearanceCircleSpacing = 5.0;  // 5mm default

        adsk::core::Ptr<adsk::core::ValueCommandInput> crossSizeInput =
            inputs->itemById("crossSize");
        if (crossSizeInput) {
            // Convert from Fusion's database units (cm) to mm
            params.crossSize = fusionLengthToMm(crossSizeInput->value());
        }

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> forceBoundary =
            inputs->itemById("forceBoundaryIntersections");
        if (forceBoundary) {
            params.forceBoundaryIntersections = forceBoundary->value();
        }

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> showMedial =
            inputs->itemById("showMedialLines");
        if (showMedial) {
            params.showMedialLines = showMedial->value();
        }

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> showClearance =
            inputs->itemById("showClearanceCircles");
        if (showClearance) {
            params.showClearanceCircles = showClearance->value();
        }

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> showPolygon =
            inputs->itemById("showPolygonizedShape");
        if (showPolygon) {
            params.showPolygonizedShape = showPolygon->value();
        }

        adsk::core::Ptr<adsk::core::BoolValueCommandInput> generateViz =
            inputs->itemById("generateVisualization");
        if (generateViz) {
            params.generateVisualization = generateViz->value();
        }

        // V-carve parameters
        adsk::core::Ptr<adsk::core::BoolValueCommandInput> generateVCarve =
            inputs->itemById("generateVCarveToolpaths");
        if (generateVCarve) {
            params.generateVCarveToolpaths = generateVCarve->value();
        }

        adsk::core::Ptr<adsk::core::ValueCommandInput> maxDepth =
            inputs->itemById("maxVCarveDepth");
        if (maxDepth) {
            // Convert from Fusion's database units (cm) to mm
            params.maxVCarveDepth = fusionLengthToMm(maxDepth->value());
        }

        // Always enable project to surface - it's the only mode we use
        params.projectToSurface = true;

        adsk::core::Ptr<adsk::core::SelectionCommandInput> surfaceSelection =
            inputs->itemById("targetSurface");
        if (surfaceSelection && surfaceSelection->selectionCount() > 0) {
            // Get the selected surface entity
            adsk::core::Ptr<adsk::core::Base> surfaceEntity = surfaceSelection->selection(0)->entity();
            if (surfaceEntity) {
                // Cast to BRepFace to get entity token
                auto face = surfaceEntity->cast<adsk::fusion::BRepFace>();
                if (face) {
                    // Store the entity token for later use
                    params.targetSurfaceId = face->entityToken();
                }
            }
        }

    } catch (...) {
        // Return default parameters on error
    }

    return params;
}

ChipCarving::Adapters::SketchSelection GeneratePathsCommandHandler::getSelectionFromInputs(
    adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
    ChipCarving::Adapters::SketchSelection selection;

    try {
        adsk::core::Ptr<adsk::core::SelectionCommandInput> profileSelection =
            inputs->itemById("sketchProfiles");
        if (profileSelection) {
            selection.closedPathCount = 0;  // Reset and count valid profiles only

            // Debug and validate each selected entity
            for (int i = 0; i < static_cast<int>(profileSelection->selectionCount()); i++) {
                adsk::core::Ptr<adsk::core::Base> entity = profileSelection->selection(i)->entity();
                if (entity) {
                    // Get entity type for debugging
                    std::string entityType = entity->objectType();

                    // With "Profiles" filter, we should only get valid Profile objects
                    // But add validation using area properties (only closed profiles have area)
                    auto profile = entity->cast<adsk::fusion::Profile>();
                    if (profile) {
                        try {
                            // Use areaProperties() as validation - only works for closed profiles
                            adsk::core::Ptr<adsk::fusion::AreaProperties> areaProps =
                                profile->areaProperties();
                            if (areaProps && areaProps->area() > 0) {
                                // Valid closed profile with measurable area
                                selection.closedPathCount++;
                                // Store the actual profile's entity token for proper identification
                                std::string profileToken = profile->entityToken();
                                selection.selectedEntityIds.push_back(profileToken);
                            } else {
                                selection.errorMessage =
                                    "Selected profile has no area (not closed)";
                                selection.isValid = false;
                                return selection;
                            }
                        } catch (...) {
                            // If areaProperties() fails, it's not a valid closed profile
                            selection.errorMessage =
                                "Selected entity is not a valid closed profile. Click INSIDE blue "
                                "shaded regions only.";
                            selection.isValid = false;
                            return selection;
                        }
                    } else {
                        // With "Profiles" filter, this shouldn't happen, but handle it
                        selection.errorMessage =
                            "FILTER ERROR: Selected entity type " + entityType +
                            " is not a Profile. The \"Profiles\" filter should prevent this.";
                        selection.isValid = false;
                        return selection;
                    }
                }
            }

            // Final validation
            selection.isValid = (selection.closedPathCount > 0);
            if (!selection.isValid) {
                if (profileSelection->selectionCount() > 0) {
                    selection.errorMessage = "No valid closed profiles found. Ensure you click "
                                             "INSIDE blue shaded regions, not on curve edges.";
                } else {
                    selection.errorMessage = "No closed profiles selected";
                }
            }
        } else {
            selection.errorMessage = "Profile selection input not found";
        }
    } catch (...) {
        selection.isValid = false;
        selection.errorMessage = "Error processing profile selection";
    }

    return selection;
}

}  // namespace Commands
}  // namespace ChipCarving

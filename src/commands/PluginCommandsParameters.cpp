/**
 * PluginCommandsParameters.cpp
 *
 * Parameter input creation and processing for PluginCommands
 * Split from PluginCommands.cpp for maintainability
 */

#include "../../include/utils/logging.h"
#include "../utils/UnitConversion.h"
#include "PluginCommands.h"

using namespace ChipCarving::Utils;

namespace ChipCarving {
namespace Commands {

void GeneratePathsCommandHandler::createParameterInputs(
    adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
  try {
    // Add wide description to make dialog wider
    adsk::core::Ptr<adsk::core::TextBoxCommandInput> titleDesc =
        inputs->addTextBoxCommandInput(
            "titleDescription", "",
            "<b>Generate V-Carve Toolpaths</b><br/>"
            "Select closed sketch profiles and configure tool parameters to "
            "generate medial axis construction geometry and V-carve toolpaths "
            "for CNC machining of chip carving patterns.",
            3, true);

    // 1. SHAPES TO CARVE (always expanded, no arrow)
    adsk::core::Ptr<adsk::core::GroupCommandInput> selectionGroup =
        inputs->addGroupCommandInput("selectionGroup", "Shapes to Carve");
    selectionGroup->isExpanded(true);
    selectionGroup->isEnabledCheckBoxDisplayed(false);
    adsk::core::Ptr<adsk::core::CommandInputs> selectionInputs =
        selectionGroup->children();

    // Selection input for sketch profiles (closed paths) with detailed tooltip
    adsk::core::Ptr<adsk::core::SelectionCommandInput> sketchSelection =
        selectionInputs->addSelectionInput("sketchProfiles",
                                           "Closed Sketch Profiles",
                                           "Select closed sketch profiles");
    // Start with all filters for sub-component support
    // The ActivateHandler will clear these and leave only "Profiles" after
    // dialog is shown
    sketchSelection->addSelectionFilter("Profiles");
    sketchSelection->addSelectionFilter("SketchCurves");
    sketchSelection->addSelectionFilter("SketchLines");
    sketchSelection->addSelectionFilter("SketchArcs");
    sketchSelection->addSelectionFilter("SketchCircles");
    sketchSelection->addSelectionFilter("SketchEllipses");
    sketchSelection->addSelectionFilter("SketchSplines");
    sketchSelection->setSelectionLimits(1, 0);  // At least 1, no upper limit

    // Set detailed tooltip with enhanced selection instructions
    sketchSelection->tooltip(
        "Select closed sketch profiles.\n\n"
        "• Click INSIDE blue shaded profile regions\n"
        "• Individual edges/curves cannot be selected\n"
        "• Use Ctrl+Click to select multiple profiles");

    // 2. V-CARVE TOOLPATHS (always expanded, no arrow)
    adsk::core::Ptr<adsk::core::GroupCommandInput> vcarveGroup =
        inputs->addGroupCommandInput("vcarveGroup", "V-Carve Toolpaths");
    vcarveGroup->isExpanded(true);
    vcarveGroup->isEnabledCheckBoxDisplayed(false);
    adsk::core::Ptr<adsk::core::CommandInputs> vcarveInputs =
        vcarveGroup->children();

    // Tool selection dropdown
    adsk::core::Ptr<adsk::core::DropDownCommandInput> toolDropdown =
        vcarveInputs->addDropDownCommandInput(
            "toolSelection", "Tool",
            adsk::core::DropDownStyles::TextListDropDownStyle);
    toolDropdown->listItems()->add("30° V-bit", false);
    toolDropdown->listItems()->add("60° V-bit", false);
    toolDropdown->listItems()->add("90° V-bit", true);  // Default selection
    toolDropdown->tooltip("Select the V-bit tool for path generation");

    adsk::core::Ptr<adsk::core::BoolValueCommandInput> generateVCarve =
        vcarveInputs->addBoolValueInput("generateVCarveToolpaths",
                                        "Generate V-Carve Toolpaths", true, "",
                                        true);
    generateVCarve->tooltip(
        "Generate 3D V-carve toolpaths from medial axis data");

    // V-carve safety parameters - FIXED UNITS
    adsk::core::Ptr<adsk::core::ValueCommandInput> maxDepth =
        vcarveInputs->addValueInput("maxVCarveDepth", "Maximum Depth", "mm",
                                    adsk::core::ValueInput::createByReal(2.5));
    maxDepth->tooltip(
        "Maximum allowed V-carve depth for safety (default: 25.0mm)");

    // Always enable project to surface - it's the only mode we use
    adsk::core::Ptr<adsk::core::SelectionCommandInput> surfaceSelection =
        vcarveInputs->addSelectionInput("targetSurface", "Target Surface",
                                        "Select surface for projection");
    surfaceSelection->addSelectionFilter(
        "Faces");  // Only allow face/surface selection
    // Note: Selection limits not available in API, user can select multiple but
    // we only use first
    surfaceSelection->tooltip(
        "Select a surface to project the V-carve toolpaths onto");

    // 3. VISUALIZATION OPTIONS (collapsible, default closed)
    adsk::core::Ptr<adsk::core::GroupCommandInput> constructionGroup =
        inputs->addGroupCommandInput("constructionGroup",
                                     "Visualization Options");
    constructionGroup->isExpanded(false);
    constructionGroup->isEnabledCheckBoxDisplayed(false);
    adsk::core::Ptr<adsk::core::CommandInputs> constructionInputs =
        constructionGroup->children();

    adsk::core::Ptr<adsk::core::BoolValueCommandInput> generateViz =
        constructionInputs->addBoolValueInput(
            "generateVisualization", "Generate Visualization", true, "", false);
    generateViz->tooltip("Generate visualization sketches (default: off)");

    adsk::core::Ptr<adsk::core::BoolValueCommandInput> showMedial =
        constructionInputs->addBoolValueInput(
            "showMedialLines", "Medial Axis Lines", true, "", true);
    showMedial->tooltip(
        "Display medial axis centerlines as construction geometry");

    adsk::core::Ptr<adsk::core::BoolValueCommandInput> showClearance =
        constructionInputs->addBoolValueInput(
            "showClearanceCircles", "Tool Clearance Circles", true, "", true);
    showClearance->tooltip(
        "Display tool clearance circles at key points along medial axis");

    adsk::core::Ptr<adsk::core::BoolValueCommandInput> showPolygon =
        constructionInputs->addBoolValueInput(
            "showPolygonizedShape", "Polygonized Boundaries", true, "", false);
    showPolygon->tooltip(
        "Display the polygon approximation used for medial axis computation");

    // Cross size - FIXED UNITS (default 0.0mm = no crosses)
    adsk::core::Ptr<adsk::core::ValueCommandInput> crossSize =
        constructionInputs->addValueInput(
            "crossSize", "Center Cross Size", "mm",
            adsk::core::ValueInput::createByReal(0.0));
    crossSize->tooltip(
        "Size of cross marks at clearance circle centers in mm (0 = no "
        "crosses, default: 0.0mm)");

    // 4. MEDIAL AXIS PARAMETERS (collapsible, default closed)
    adsk::core::Ptr<adsk::core::GroupCommandInput> paramGroup =
        inputs->addGroupCommandInput("paramGroup", "Medial Axis Parameters");
    paramGroup->isExpanded(false);
    paramGroup->isEnabledCheckBoxDisplayed(false);
    adsk::core::Ptr<adsk::core::CommandInputs> groupInputs =
        paramGroup->children();

    // Polygon tolerance (max allowed error when approximating curves) - FIXED
    // UNITS Fusion 360 internal units are cm, so 0.25mm = 0.025cm
    adsk::core::Ptr<adsk::core::ValueCommandInput> polygonTol =
        groupInputs->addValueInput("polygonTolerance",
                                   "Polygon Error Tolerance", "mm",
                                   adsk::core::ValueInput::createByReal(0.025));
    polygonTol->tooltip(
        "Maximum allowed error when approximating curved edges with line "
        "segments (default: 0.25mm)");

    // Force boundary intersections
    adsk::core::Ptr<adsk::core::BoolValueCommandInput> forceBoundary =
        groupInputs->addBoolValueInput("forceBoundaryIntersections",
                                       "Force Boundary Intersections", true, "",
                                       true);
    forceBoundary->tooltip(
        "Ensure every intersection with shape boundary is included in the "
        "path");

    // Surface sampling distance - controls V-carve path density for better
    // surface following Fusion 360 internal units are cm, so 2.0mm = 0.2cm
    adsk::core::Ptr<adsk::core::ValueCommandInput> samplingDistance =
        groupInputs->addValueInput("surfaceSamplingDistance",
                                   "Surface Sampling Distance", "mm",
                                   adsk::core::ValueInput::createByReal(0.2));
    samplingDistance->tooltip(
        "Distance between V-carve points for surface following (smaller = more "
        "accurate, default: 2.0mm)");
  } catch (...) {
    // Ignore input creation errors for now
  }
}

ChipCarving::Adapters::MedialAxisParameters
GeneratePathsCommandHandler::getParametersFromInputs(
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
      params.samplingDistance =
          fusionLengthToMm(samplingDistanceInput->value());
    } else {
      // Fallback to default if input not found
      params.samplingDistance = 2.0;  // 2mm default
    }

    // REMOVED: Reading clearanceCircleSpacing - no longer needed
    // Set default clearance circle spacing (not used, but may be expected by
    // other code)
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
      adsk::core::Ptr<adsk::core::Base> surfaceEntity =
          surfaceSelection->selection(0)->entity();
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

ChipCarving::Adapters::SketchSelection
GeneratePathsCommandHandler::getSelectionFromInputs(
    adsk::core::Ptr<adsk::core::CommandInputs> inputs) {
  ChipCarving::Adapters::SketchSelection selection;

  try {
    // IMMEDIATE EXTRACTION APPROACH: Use cached geometry instead of processing
    // selections
    LOG_INFO(
        "Using cached geometry from immediate extraction. Available cached "
        "profiles: " << cachedProfiles_.size());

    if (!cachedProfiles_.empty()) {
      // Use the cached geometry that was extracted immediately when selections
      // were made
      selection.isValid = true;
      selection.closedPathCount = cachedProfiles_.size();
      selection.selectedProfiles = cachedProfiles_;

      // Generate dummy entity IDs for backward compatibility (these won't be
      // used)
      for (size_t i = 0; i < cachedProfiles_.size(); ++i) {
        selection.selectedEntityIds.push_back("cached_profile_" +
                                              std::to_string(i));
      }

      LOG_INFO("Successfully using " << cachedProfiles_.size()
                                     << " cached profiles");
      return selection;
    }

    // FALLBACK: If no cached geometry, try the original approach
    LOG_INFO(
        "No cached geometry available, falling back to original selection "
        "processing");

    adsk::core::Ptr<adsk::core::SelectionCommandInput> profileSelection =
        inputs->itemById("sketchProfiles");
    if (profileSelection) {
      selection.closedPathCount = 0;  // Reset and count valid profiles only

      // First, separate selections into profiles and curves
      std::vector<adsk::core::Ptr<adsk::fusion::Profile>> directProfiles;
      std::map<std::string,
               std::vector<adsk::core::Ptr<adsk::fusion::SketchCurve>>>
          curvesBySketch;

      // Categorize each selected entity
      LOG_INFO("Processing " << profileSelection->selectionCount()
                             << " selected entities");
      for (int i = 0; i < static_cast<int>(profileSelection->selectionCount());
           i++) {
        adsk::core::Ptr<adsk::core::Base> entity =
            profileSelection->selection(i)->entity();
        if (entity) {
          std::string entityType = entity->objectType();
          // Try to cast as Profile first (for root component selections)
          auto profile = entity->cast<adsk::fusion::Profile>();
          if (profile) {
            LOG_INFO("Selection " << i << ": Profile from sketch '"
                                  << (profile->parentSketch()
                                          ? profile->parentSketch()->name()
                                          : "unknown")
                                  << "'");
            directProfiles.push_back(profile);
          } else {
            // Try to cast as SketchCurve (for sub-component selections)
            auto sketchCurve = entity->cast<adsk::fusion::SketchCurve>();
            if (sketchCurve && sketchCurve->parentSketch()) {
              std::string sketchName = sketchCurve->parentSketch()->name();
              std::string sketchId = sketchCurve->parentSketch()->entityToken();
              LOG_INFO("Selection " << i << ": " << entityType
                                    << " from sketch '" << sketchName << "'");
              curvesBySketch[sketchId].push_back(sketchCurve);
            } else {
              LOG_INFO("Selection " << i << ": Unhandled entity type '"
                                    << entityType << "'");
            }
          }
        }
      }

      // Process direct profile selections (root component)
      for (const auto& profile : directProfiles) {
        try {
          // Use areaProperties() as validation - only works for closed profiles
          adsk::core::Ptr<adsk::fusion::AreaProperties> areaProps =
              profile->areaProperties();
          if (areaProps && areaProps->area() > 0) {
            // Valid closed profile with measurable area
            selection.closedPathCount++;

            // Store the entity token for backward compatibility
            std::string profileToken = profile->entityToken();
            selection.selectedEntityIds.push_back(profileToken);

            // NEW: Store profile metadata immediately INCLUDING the actual
            // profile object This allows us to extract geometry directly
            // without relying on tokens
            ChipCarving::Adapters::ProfileGeometry profileGeom;

            // Store basic profile data
            profileGeom.area = areaProps->area();
            auto centroid = areaProps->centroid();
            if (centroid) {
              profileGeom.centroid = {centroid->x(), centroid->y()};
            }

            // Get sketch name if available
            if (profile->parentSketch()) {
              profileGeom.sketchName = profile->parentSketch()->name();

              // Get plane entity ID
              auto referenceEntity = profile->parentSketch()->referencePlane();
              if (referenceEntity) {
                auto constructionPlane =
                    referenceEntity->cast<adsk::fusion::ConstructionPlane>();
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

            // NOTE: Vertices will be extracted later via existing
            // extractProfileVertices This two-phase approach maintains
            // compatibility while fixing the stale token issue

            // Add to selected profiles
            selection.selectedProfiles.push_back(profileGeom);
          } else {
            selection.errorMessage =
                "Selected profile has no area (not closed)";
            selection.isValid = false;
            return selection;
          }
        } catch (...) {
          // If areaProperties() fails, it's not a valid closed profile
          selection.errorMessage =
              "Selected entity is not a valid closed profile. Click INSIDE "
              "blue "
              "shaded regions only.";
          selection.isValid = false;
          return selection;
        }
      }

      // Process curve selections (sub-components)
      // Check if curves from each sketch form a closed loop
      LOG_INFO("Processing curves from " << curvesBySketch.size()
                                         << " sketches");
      for (const auto& sketchCurvePair : curvesBySketch) {
        const auto& sketchId = sketchCurvePair.first;
        const auto& curves = sketchCurvePair.second;
        if (curves.empty()) continue;

        // Get the parent sketch from the first curve
        auto sketch = curves[0]->parentSketch();
        if (!sketch) continue;

        LOG_INFO("Checking " << curves.size() << " curves from sketch '"
                             << sketch->name() << "' for complete profiles");

        // Check if these curves form a closed loop
        // For now, we'll check if we have all the curves from any profile in
        // this sketch
        bool foundCompleteProfile = false;

        // Get all profiles in this sketch
        auto sketchProfiles = sketch->profiles();
        if (sketchProfiles) {
          LOG_INFO("Sketch has " << sketchProfiles->count() << " profiles");
          for (int p = 0; p < sketchProfiles->count(); ++p) {
            auto candidateProfile = sketchProfiles->item(p);
            if (!candidateProfile) continue;

            // Count how many curves from this profile are selected
            int matchingCurves = 0;
            int totalCurvesInProfile = 0;

            // Check each loop in the profile
            auto profileLoops = candidateProfile->profileLoops();
            if (profileLoops) {
              for (int l = 0; l < profileLoops->count(); ++l) {
                auto loop = profileLoops->item(l);
                if (!loop) continue;

                auto loopCurves = loop->profileCurves();
                if (loopCurves) {
                  totalCurvesInProfile += loopCurves->count();

                  // Check if selected curves match this loop's curves
                  for (int c = 0; c < loopCurves->count(); ++c) {
                    auto profileCurve = loopCurves->item(c);
                    if (profileCurve && profileCurve->sketchEntity()) {
                      auto curveEntity = profileCurve->sketchEntity();

                      // Check if this curve is in our selected curves
                      for (const auto& selectedCurve : curves) {
                        if (curveEntity->entityToken() ==
                            selectedCurve->entityToken()) {
                          matchingCurves++;
                          break;
                        }
                      }
                    }
                  }
                }
              }
            }

            LOG_INFO("Profile " << p << ": " << matchingCurves << "/"
                                << totalCurvesInProfile
                                << " curves match selection");

            // If all curves from this profile are selected, we have a complete
            // profile
            if (matchingCurves == totalCurvesInProfile &&
                totalCurvesInProfile > 0) {
              LOG_INFO("Found complete profile match!");
              // We found a complete profile! Process it like a direct profile
              // selection
              try {
                adsk::core::Ptr<adsk::fusion::AreaProperties> areaProps =
                    candidateProfile->areaProperties();
                if (areaProps && areaProps->area() > 0) {
                  selection.closedPathCount++;

                  std::string profileToken = candidateProfile->entityToken();
                  selection.selectedEntityIds.push_back(profileToken);

                  ChipCarving::Adapters::ProfileGeometry profileGeom;
                  profileGeom.area = areaProps->area();
                  auto centroid = areaProps->centroid();
                  if (centroid) {
                    profileGeom.centroid = {centroid->x(), centroid->y()};
                  }

                  profileGeom.sketchName = sketch->name();

                  auto referenceEntity = sketch->referencePlane();
                  if (referenceEntity) {
                    auto constructionPlane =
                        referenceEntity
                            ->cast<adsk::fusion::ConstructionPlane>();
                    if (constructionPlane) {
                      profileGeom.planeEntityId =
                          constructionPlane->entityToken();
                    } else {
                      auto face =
                          referenceEntity->cast<adsk::fusion::BRepFace>();
                      if (face) {
                        profileGeom.planeEntityId = face->entityToken();
                      }
                    }
                  }

                  selection.selectedProfiles.push_back(profileGeom);
                  foundCompleteProfile = true;
                  break;
                }
              } catch (...) {
                // Profile validation failed
              }
            }
          }
        }

        if (!foundCompleteProfile) {
          // The selected curves don't form a complete profile
          selection.errorMessage =
              "Selected curves do not form a complete closed profile. "
              "For sub-components, you must select ALL curves that form the "
              "profile.";
          selection.isValid = false;
          return selection;
        }
      }

      // Final validation
      LOG_INFO("Total valid profiles found: " << selection.closedPathCount);
      selection.isValid = (selection.closedPathCount > 0);
      if (!selection.isValid) {
        if (profileSelection->selectionCount() > 0) {
          selection.errorMessage =
              "No valid closed profiles found. Ensure you click "
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
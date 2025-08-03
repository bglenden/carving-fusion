/**
 * FusionWorkspaceSketch.cpp
 *
 * Main sketch operations file that includes all split sketch components
 * Split for maintainability - see individual files for implementations
 */

// Include all split implementation files
#include "FusionWorkspaceSketchBasic.cpp"
#include "FusionWorkspaceSketchComponent.cpp"
#include "FusionWorkspaceSketchPlane.cpp"

using namespace adsk::core;

namespace ChipCarving {
namespace Adapters {

// Constructor - shared across all split files
FusionWorkspace::FusionWorkspace(Ptr<Application> app) : app_(app) {}

}  // namespace Adapters
}  // namespace ChipCarving

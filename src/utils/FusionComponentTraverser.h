/**
 * FusionComponentTraverser.h
 *
 * Utility to abstract common Fusion 360 component traversal patterns
 * Eliminates duplication of allOccurrences() loops found in 3+ files
 */

#pragma once

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <functional>
#include <vector>

namespace ChipCarving {
namespace Utils {

/**
 * Callback function type for component processing
 * Parameters: component, componentIndex
 * Returns: true to continue traversal, false to stop
 */
using ComponentCallback =
    std::function<bool(adsk::core::Ptr<adsk::fusion::Component>, size_t)>;

/**
 * Utility class to abstract common Fusion 360 component traversal patterns
 *
 * Usage:
 *   auto traverser = FusionComponentTraverser(rootComponent);
 *   auto allComponents = traverser.getAllComponents();
 *
 *   // Or with callback:
 *   traverser.forEachComponent([](auto comp, size_t idx) {
 *       // Process component
 *       return true;  // continue
 *   });
 */
class FusionComponentTraverser {
 public:
  explicit FusionComponentTraverser(
      adsk::core::Ptr<adsk::fusion::Component> rootComponent);

  // Get all components (root + all occurrences) as a vector
  std::vector<adsk::core::Ptr<adsk::fusion::Component>> getAllComponents();

  // Apply callback to each component (root + all occurrences)
  void forEachComponent(ComponentCallback callback);

  // Count total components
  size_t getComponentCount();

  // Search for specific component by criteria
  adsk::core::Ptr<adsk::fusion::Component> findComponent(
      std::function<bool(adsk::core::Ptr<adsk::fusion::Component>)> predicate);

  // Get components containing specific entity type
  template <typename T>
  std::vector<adsk::core::Ptr<adsk::fusion::Component>>
  getComponentsContaining();

 private:
  adsk::core::Ptr<adsk::fusion::Component> rootComponent_;

  void collectComponents(
      std::vector<adsk::core::Ptr<adsk::fusion::Component>>& components);
};

// Template specializations for common entity types
template <>
std::vector<adsk::core::Ptr<adsk::fusion::Component>>
FusionComponentTraverser::getComponentsContaining<adsk::fusion::Sketches>();

template <>
std::vector<adsk::core::Ptr<adsk::fusion::Component>>
FusionComponentTraverser::getComponentsContaining<adsk::fusion::BRepBodies>();

}  // namespace Utils
}  // namespace ChipCarving


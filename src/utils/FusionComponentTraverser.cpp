/**
 * FusionComponentTraverser.cpp
 *
 * Implementation of Fusion 360 component traversal utility
 */

#include "FusionComponentTraverser.h"
#include "DebugLogger.h"

namespace ChipCarving {
namespace Utils {

FusionComponentTraverser::FusionComponentTraverser(adsk::core::Ptr<adsk::fusion::Component> rootComponent)
    : rootComponent_(rootComponent) {

    auto logger = DebugLogger::getInstance();
    if (!rootComponent_) {
        logger->logError("FusionComponentTraverser initialized with null root component");
    } else {
        logger->logDebug("FusionComponentTraverser initialized successfully");
    }
}

std::vector<adsk::core::Ptr<adsk::fusion::Component>> FusionComponentTraverser::getAllComponents() {
    std::vector<adsk::core::Ptr<adsk::fusion::Component>> components;
    collectComponents(components);

    auto logger = DebugLogger::getInstance();
    logger->logDebug("Found " + std::to_string(components.size()) + " total components");

    return components;
}

void FusionComponentTraverser::forEachComponent(ComponentCallback callback) {
    if (!callback || !rootComponent_) {
        return;
    }

    auto components = getAllComponents();

    for (size_t i = 0; i < components.size(); ++i) {
        if (!callback(components[i], i)) {
            // Callback returned false, stop traversal
            break;
        }
    }
}

size_t FusionComponentTraverser::getComponentCount() {
    if (!rootComponent_) {
        return 0;
    }

    size_t count = 1;  // Root component

    auto occurrences = rootComponent_->allOccurrences();
    if (occurrences) {
        count += occurrences->count();
    }

    return count;
}

adsk::core::Ptr<adsk::fusion::Component> FusionComponentTraverser::findComponent(
    std::function<bool(adsk::core::Ptr<adsk::fusion::Component>)> predicate) {

    if (!predicate) {
        return nullptr;
    }

    auto components = getAllComponents();

    for (auto component : components) {
        if (component && predicate(component)) {
            return component;
        }
    }

    return nullptr;
}

void FusionComponentTraverser::collectComponents(std::vector<adsk::core::Ptr<adsk::fusion::Component>>& components) {
    auto logger = DebugLogger::getInstance();

    if (!rootComponent_) {
        logger->logError("Cannot collect components - root component is null");
        return;
    }

    // Add root component first
    components.push_back(rootComponent_);

    // Add all occurrences (sub-components)
    auto occurrences = rootComponent_->allOccurrences();
    if (occurrences) {
        logger->logDebug("Found " + std::to_string(occurrences->count()) + " component occurrences");

        for (size_t i = 0; i < occurrences->count(); ++i) {
            auto occurrence = occurrences->item(i);
            if (occurrence && occurrence->component()) {
                components.push_back(occurrence->component());
            } else {
                logger->logWarning("Skipping invalid occurrence at index " + std::to_string(i));
            }
        }
    } else {
        logger->logDebug("No component occurrences found");
    }
}

// Template specialization for Sketches
template<>
std::vector<adsk::core::Ptr<adsk::fusion::Component>>
FusionComponentTraverser::getComponentsContaining<adsk::fusion::Sketches>() {
    std::vector<adsk::core::Ptr<adsk::fusion::Component>> result;

    forEachComponent([&result](auto component, size_t index) {
        (void)index;  // Suppress unused parameter warning

        if (component) {
            auto sketches = component->sketches();
            if (sketches && sketches->count() > 0) {
                result.push_back(component);
            }
        }
        return true;  // Continue traversal
    });

    return result;
}

// Template specialization for BRepBodies
template<>
std::vector<adsk::core::Ptr<adsk::fusion::Component>>
FusionComponentTraverser::getComponentsContaining<adsk::fusion::BRepBodies>() {
    std::vector<adsk::core::Ptr<adsk::fusion::Component>> result;

    forEachComponent([&result](auto component, size_t index) {
        (void)index;  // Suppress unused parameter warning

        if (component) {
            auto bodies = component->bRepBodies();
            if (bodies && bodies->count() > 0) {
                result.push_back(component);
            }
        }
        return true;  // Continue traversal
    });

    return result;
}

}  // namespace Utils
}  // namespace ChipCarving

/**
 * UIParameterHelper.cpp
 *
 * Implementation of UI parameter access utility
 */

#include "UIParameterHelper.h"
#include "../../include/utils/logging.h"
#include "UnitConversion.h"

namespace ChipCarving {
namespace Utils {

UIParameterHelper::UIParameterHelper(adsk::core::Ptr<adsk::core::CommandInputs> inputs)
    : inputs_(inputs) {

    if (!inputs_) {
        LOG_ERROR("UIParameterHelper initialized with null inputs");
    } else {
        LOG_INFO("UIParameterHelper initialized with " << inputs_->count() << " inputs");
    }
}

void UIParameterHelper::logParameterAccess(const std::string& inputId, bool success) const {
    if (success) {
        LOG_DEBUG("Successfully accessed parameter: " << inputId);
    } else {
        LOG_WARNING("Failed to access parameter: " << inputId);
    }
}

// Template specializations
template<>
adsk::core::Ptr<adsk::core::StringValueCommandInput> 
UIParameterHelper::getInput<adsk::core::StringValueCommandInput>(const std::string& inputId) {
    if (!inputs_) return nullptr;
    return inputs_->itemById(inputId);
}

template<>
adsk::core::Ptr<adsk::core::BoolValueCommandInput> 
UIParameterHelper::getInput<adsk::core::BoolValueCommandInput>(const std::string& inputId) {
    if (!inputs_) return nullptr;
    return inputs_->itemById(inputId);
}

template<>
adsk::core::Ptr<adsk::core::ValueCommandInput> 
UIParameterHelper::getInput<adsk::core::ValueCommandInput>(const std::string& inputId) {
    if (!inputs_) return nullptr;
    return inputs_->itemById(inputId);
}

template<>
adsk::core::Ptr<adsk::core::IntegerSpinnerCommandInput> 
UIParameterHelper::getInput<adsk::core::IntegerSpinnerCommandInput>(const std::string& inputId) {
    if (!inputs_) return nullptr;
    return inputs_->itemById(inputId);
}

template<>
adsk::core::Ptr<adsk::core::SelectionCommandInput> 
UIParameterHelper::getInput<adsk::core::SelectionCommandInput>(const std::string& inputId) {
    if (!inputs_) return nullptr;
    return inputs_->itemById(inputId);
}

// Value getter implementations
std::string UIParameterHelper::getStringValue(const std::string& inputId) {
    auto stringInput = getInput<adsk::core::StringValueCommandInput>(inputId);
    if (stringInput) {
        logParameterAccess(inputId, true);
        return stringInput->value();
    }

    logParameterAccess(inputId, false);
    return "";
}

bool UIParameterHelper::getBoolValue(const std::string& inputId) {
    auto boolInput = getInput<adsk::core::BoolValueCommandInput>(inputId);
    if (boolInput) {
        logParameterAccess(inputId, true);
        return boolInput->value();
    }

    logParameterAccess(inputId, false);
    return false;
}

double UIParameterHelper::getDoubleValue(const std::string& inputId) {
    auto valueInput = getInput<adsk::core::ValueCommandInput>(inputId);
    if (valueInput) {
        logParameterAccess(inputId, true);
        return valueInput->value();
    }

    logParameterAccess(inputId, false);
    return 0.0;
}

int UIParameterHelper::getIntegerValue(const std::string& inputId) {
    auto integerInput = getInput<adsk::core::IntegerSpinnerCommandInput>(inputId);
    if (integerInput) {
        logParameterAccess(inputId, true);
        return integerInput->value();
    }

    logParameterAccess(inputId, false);
    return 0;
}

std::optional<std::string> UIParameterHelper::getDropdownValue(const std::string& inputId) {
    auto dropdownInput = inputs_->itemById(inputId);
    if (!dropdownInput) {
        logParameterAccess(inputId, false);
        return std::nullopt;
    }

    auto dropdown = dropdownInput->cast<adsk::core::DropDownCommandInput>();
    if (!dropdown) {
        logParameterAccess(inputId, false);
        return std::nullopt;
    }

    auto selectedItem = dropdown->selectedItem();
    if (!selectedItem) {
        logParameterAccess(inputId, false);
        return std::nullopt;
    }

    logParameterAccess(inputId, true);
    return selectedItem->name();
}

}  // namespace Utils
}  // namespace ChipCarving

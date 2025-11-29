/**
 * UIParameterHelper.h
 *
 * Utility class for type-safe access to Fusion 360 UI command inputs
 * Provides template-based parameter extraction with compile-time type safety
 */

#pragma once

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <string>

#include "utils/Optional.h"

namespace ChipCarving {
namespace Utils {

/**
 * Helper class for extracting typed parameters from Fusion 360 command inputs
 * Uses template specialization to provide type-safe access to different input
 * types
 */
class UIParameterHelper {
 public:
  /**
   * Constructor - takes the command inputs collection
   * @param inputs The command inputs collection from the command event
   */
  explicit UIParameterHelper(adsk::core::Ptr<adsk::core::CommandInputs> inputs);

  /**
   * Generic template method for getting typed input by ID
   * Template specializations provide type-safe access for specific input types
   * @param inputId The ID of the input to retrieve
   * @return Typed pointer to the input, or nullptr if not found/wrong type
   */
  template <typename T>
  adsk::core::Ptr<T> getInput(const std::string& inputId) {
    (void)inputId;  // Suppress unused parameter warning - used in specializations
    // Default implementation returns nullptr - specializations provide actual
    // functionality
    return nullptr;
  }

  /**
   * Get string value from a StringValueCommandInput
   * @param inputId The ID of the string input
   * @return The string value, or empty string if input not found
   */
  std::string getStringValue(const std::string& inputId);

  /**
   * Get boolean value from a BoolValueCommandInput
   * @param inputId The ID of the boolean input
   * @return The boolean value, or false if input not found
   */
  bool getBoolValue(const std::string& inputId);

  /**
   * Get double value from a ValueCommandInput
   * @param inputId The ID of the value input
   * @return The double value, or 0.0 if input not found
   */
  double getDoubleValue(const std::string& inputId);

  /**
   * Get integer value from an IntegerSpinnerCommandInput
   * @param inputId The ID of the integer input
   * @return The integer value, or 0 if input not found
   */
  int getIntegerValue(const std::string& inputId);

  /**
   * Get dropdown selection from a DropDownCommandInput
   * @param inputId The ID of the dropdown input
   * @return The selected item name, or empty optional if not found
   */
  Optional<std::string> getDropdownValue(const std::string& inputId);

 private:
  adsk::core::Ptr<adsk::core::CommandInputs> inputs_;

  /**
   * Helper method to log parameter access attempts
   * @param inputId The parameter ID being accessed
   * @param success Whether the access was successful
   */
  void logParameterAccess(const std::string& inputId, bool success) const;
};

// Template specializations for specific input types

/**
 * Specialization for StringValueCommandInput
 */
template <>
adsk::core::Ptr<adsk::core::StringValueCommandInput> UIParameterHelper::getInput<adsk::core::StringValueCommandInput>(
    const std::string& inputId);

/**
 * Specialization for BoolValueCommandInput
 */
template <>
adsk::core::Ptr<adsk::core::BoolValueCommandInput> UIParameterHelper::getInput<adsk::core::BoolValueCommandInput>(
    const std::string& inputId);

/**
 * Specialization for ValueCommandInput
 */
template <>
adsk::core::Ptr<adsk::core::ValueCommandInput> UIParameterHelper::getInput<adsk::core::ValueCommandInput>(
    const std::string& inputId);

/**
 * Specialization for IntegerSpinnerCommandInput
 */
template <>
adsk::core::Ptr<adsk::core::IntegerSpinnerCommandInput>
UIParameterHelper::getInput<adsk::core::IntegerSpinnerCommandInput>(const std::string& inputId);

/**
 * Specialization for SelectionCommandInput
 */
template <>
adsk::core::Ptr<adsk::core::SelectionCommandInput> UIParameterHelper::getInput<adsk::core::SelectionCommandInput>(
    const std::string& inputId);

}  // namespace Utils
}  // namespace ChipCarving

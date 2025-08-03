/**
 * Unit conversion utilities for Fusion 360 API
 *
 * Fusion 360 API always uses database units internally:
 * - Lengths: centimeters (cm)
 * - Angles: radians
 *
 * This header provides utilities to convert between Fusion's database units
 * and the units expected by the plugin's business logic.
 */

#pragma once

namespace ChipCarving {
namespace Utils {

/**
 * Convert length from Fusion's database units (cm) to millimeters
 * @param lengthInCm Length value in centimeters (from Fusion API)
 * @return Length value in millimeters
 */
inline double fusionLengthToMm(double lengthInCm) { return lengthInCm * 10.0; }

/**
 * Convert length from millimeters to Fusion's database units (cm)
 * @param lengthInMm Length value in millimeters
 * @return Length value in centimeters (for Fusion API)
 */
inline double mmToFusionLength(double lengthInMm) { return lengthInMm / 10.0; }

/**
 * Convert angle from Fusion's database units (radians) to degrees
 * @param angleInRadians Angle value in radians (from Fusion API)
 * @return Angle value in degrees
 */
inline double fusionAngleToDegrees(double angleInRadians) {
  return angleInRadians * 180.0 / 3.14159265358979323846;
}

/**
 * Convert angle from degrees to Fusion's database units (radians)
 * @param angleInDegrees Angle value in degrees
 * @return Angle value in radians (for Fusion API)
 */
inline double degreesToFusionAngle(double angleInDegrees) {
  return angleInDegrees * 3.14159265358979323846 / 180.0;
}

}  // namespace Utils
}  // namespace ChipCarving

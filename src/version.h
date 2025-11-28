/**
 * Version tracking for Chip Carving Paths C++ Plugin
 *
 * Uses semantic versioning (semver.org):
 * - Release builds: "1.0.1"
 * - Pre-release builds: "1.0.1-dev+abc1234" (includes git hash)
 */

#pragma once

#define ADDIN_VERSION_MAJOR 1
#define ADDIN_VERSION_MINOR 0
#define ADDIN_VERSION_PATCH 1

// Full version string following semver convention
// Pre-release: "1.0.1-dev+abc1234"
// Release: "1.0.1"
#define ADDIN_VERSION_STRING "1.0.1-dev+b0d8370"

#define ADDIN_NAME "Chip Carving Paths C++"

// Build information
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Version history
// 0.1.x - Initial C++ implementation
// 0.3.x - Enhanced features and stability
// 1.0.0 - First stable release with semantic versioning
// 1.0.1-dev - API improvements (findEntityByToken refactoring)

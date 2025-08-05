/**
 * Version tracking for Chip Carving Paths C++ Plugin
 * Auto-increment minor version on each build
 */

#pragma once

#define ADDIN_VERSION_MAJOR 0
#define ADDIN_VERSION_MINOR 9
#define ADDIN_VERSION_PATCH 223

// Dynamic version string based on build type
#define CMAKE_BUILD_TYPE_DEBUG 0
#if CMAKE_BUILD_TYPE_DEBUG
#define ADDIN_VERSION_STRING "0.9.223-debug"
#else
#define ADDIN_VERSION_STRING "0.9.223-release"
#endif

#define ADDIN_NAME "Chip Carving Paths C++"

// Build information
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Version history (cleaned up - removed repetitive auto-generated comments)
// 0.1.0 - Initial C++ implementation with basic commands
// 0.1.1 - Added debug mode and comprehensive logging
// 0.1.2 - Added toolbar panel support
// 0.1.3 - Added icons and version tracking
// 0.1.6 - Added JSON parsing functionality for Import Design command
// 0.3.115 - Current version with enhanced features and stability improvements

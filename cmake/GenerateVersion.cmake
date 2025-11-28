# GenerateVersion.cmake - Process version.h.in template at build time
# Supports semantic versioning with pre-release tags and git hash metadata

# Read version from VERSION file
file(READ "${SOURCE_DIR}/VERSION" VERSION_CONTENT)
string(STRIP "${VERSION_CONTENT}" VERSION_CONTENT)

# Parse version: supports formats like "1.0.1" or "1.0.1-dev"
# Match: MAJOR.MINOR.PATCH with optional -PRERELEASE
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-[a-zA-Z0-9.]+)?" _ "${VERSION_CONTENT}")
set(ADDIN_VERSION_MAJOR ${CMAKE_MATCH_1})
set(ADDIN_VERSION_MINOR ${CMAKE_MATCH_2})
set(ADDIN_VERSION_PATCH ${CMAKE_MATCH_3})
set(ADDIN_VERSION_PRERELEASE "${CMAKE_MATCH_4}")

# Get git short hash for build metadata
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHORT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_RESULT
)

# If git command failed, use "unknown"
if(NOT GIT_RESULT EQUAL 0)
    set(GIT_SHORT_HASH "unknown")
endif()

# Build the full version string per semver spec:
# - Release: "1.0.1"
# - Pre-release: "1.0.1-dev"
# - With build metadata: "1.0.1-dev+abc1234"
set(ADDIN_VERSION_CORE "${ADDIN_VERSION_MAJOR}.${ADDIN_VERSION_MINOR}.${ADDIN_VERSION_PATCH}")

if(ADDIN_VERSION_PRERELEASE)
    # Pre-release version: include git hash as build metadata
    set(ADDIN_VERSION_STRING "${ADDIN_VERSION_CORE}${ADDIN_VERSION_PRERELEASE}+${GIT_SHORT_HASH}")
else()
    # Release version: clean semver, no suffixes
    set(ADDIN_VERSION_STRING "${ADDIN_VERSION_CORE}")
endif()

# Set build type debug flag (for backward compatibility)
if(NOT DEFINED CMAKE_BUILD_TYPE_DEBUG)
    set(CMAKE_BUILD_TYPE_DEBUG 0)
endif()

# Process the template
configure_file(
    "${SOURCE_DIR}/src/version.h.in"
    "${SOURCE_DIR}/src/version.h"
    @ONLY
)

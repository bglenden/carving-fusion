cmake_minimum_required(VERSION 3.20)
project(ChipCarvingPathsCppSimple)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type - default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Debug flags - full symbols, no optimization, frame pointers for better stack traces
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG -D_DEBUG -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Enable all assertions and debug checks
add_definitions(-DDEBUG -D_DEBUG -UNDEBUG)

# Find Fusion 360 SDK
set(FUSION_SDK_PATH "$ENV{HOME}/Library/Application Support/Autodesk/Autodesk Fusion 360/API/CPP")
message(STATUS "Using Fusion SDK at: ${FUSION_SDK_PATH}")

# Find OpenVoronoi library (system installation)
set(OPENVORONOI_INCLUDE_DIR "/usr/local/include/openvoronoi")
set(OPENVORONOI_LIBRARY_DIR "/usr/local/lib/openvoronoi")

# Verify OpenVoronoi library exists
find_library(OPENVORONOI_LIBRARY 
    NAMES openvoronoi
    PATHS ${OPENVORONOI_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

if(NOT OPENVORONOI_LIBRARY)
    message(FATAL_ERROR "OpenVoronoi library not found at ${OPENVORONOI_LIBRARY_DIR}")
endif()

message(STATUS "Found OpenVoronoi library: ${OPENVORONOI_LIBRARY}")
message(STATUS "Using OpenVoronoi headers: ${OPENVORONOI_INCLUDE_DIR}")

# Include directories
include_directories(${FUSION_SDK_PATH}/include)
include_directories(SYSTEM ${OPENVORONOI_INCLUDE_DIR})

# Auto-increment version before building
add_custom_target(increment_version
    COMMAND ${CMAKE_COMMAND} -E echo "Auto-incrementing version..."
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/increment_version.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Incrementing version number"
)

# Read version from VERSION file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_CONTENT)
    string(STRIP "${VERSION_CONTENT}" VERSION_CONTENT)
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${VERSION_CONTENT})
    set(ADDIN_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(ADDIN_VERSION_MINOR ${CMAKE_MATCH_2})
    set(ADDIN_VERSION_PATCH ${CMAKE_MATCH_3})
else()
    # Fallback if VERSION file doesn't exist
    set(ADDIN_VERSION_MAJOR 0)
    set(ADDIN_VERSION_MINOR 3)
    set(ADDIN_VERSION_PATCH 220)
endif()

# Set build type flag for template
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE_DEBUG 1)
else()
    set(CMAKE_BUILD_TYPE_DEBUG 0)
endif()

# Generate version header after incrementing version
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h
    COMMAND ${CMAKE_COMMAND} 
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DADDIN_VERSION_MAJOR=${ADDIN_VERSION_MAJOR}
        -DADDIN_VERSION_MINOR=${ADDIN_VERSION_MINOR}
        -DADDIN_VERSION_PATCH=${ADDIN_VERSION_PATCH}
        -DCMAKE_BUILD_TYPE_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersion.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in increment_version
    COMMENT "Generating version.h from template"
)

# Create the main add-in shared library with full functionality
add_library(chip_carving_paths_cpp SHARED 
    src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h
    src/core/PluginManager.cpp
    src/core/PluginInitializer.cpp
    src/adapters/FusionAPIAdapter.cpp
    src/adapters/FusionLogger.cpp
    src/adapters/FusionWorkspace.cpp
    src/adapters/FusionWorkspaceProfileSearch.cpp
    src/adapters/FusionWorkspaceCurveExtraction.cpp
    src/adapters/FusionSketch.cpp
    src/adapters/FusionAPIFactory.cpp
    src/commands/PluginCommands.cpp
    src/parsers/DesignParser.cpp
    src/geometry/ShapeFactory.cpp
    src/geometry/Leaf.cpp
    src/geometry/TriArc.cpp
    src/geometry/MedialAxisUtilities.cpp
    src/geometry/MedialAxisProcessor.cpp
    src/geometry/VCarvePath.cpp
    src/geometry/VCarveCalculator.cpp
    src/utils/logging.cpp
    src/utils/TempFileManager.cpp
    src/utils/DebugLogger.cpp
    src/utils/FusionComponentTraverser.cpp
    src/utils/UIParameterHelper.cpp
    src/utils/ErrorHandler.cpp
)

# Make sure version is incremented before building the main target
add_dependencies(chip_carving_paths_cpp increment_version)

# Debug version now uses the same main with environment variable control
# Set CHIP_CARVING_PLUGIN_MODE=DEBUG to enable debug mode

# Set properties for macOS
if(APPLE)
    set_target_properties(chip_carving_paths_cpp PROPERTIES
        SUFFIX ".dylib"
        PREFIX ""
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@loader_path/../../../..;@loader_path/../../../../Libraries"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Find Boost for OpenVoronoi (needed for MedialAxisProcessor)
find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Using Boost for main plugin: ${Boost_INCLUDE_DIRS}")
endif()

# Link Fusion 360 libraries and OpenVoronoi
target_link_libraries(chip_carving_paths_cpp
    ${FUSION_SDK_PATH}/lib/core.dylib
    ${FUSION_SDK_PATH}/lib/fusion.dylib
    ${OPENVORONOI_LIBRARY}
    ${Boost_LIBRARIES}
)


# Set output directory
set_target_properties(chip_carving_paths_cpp PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)


# Copy resources directory to output
add_custom_command(TARGET chip_carving_paths_cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
    COMMENT "Copying resources directory"
)

# Standard install target using CMake's install command
# This will be invoked by 'make install' - properly install to Fusion 360
set(FUSION_ADDINS_DIR "$ENV{HOME}/Library/Application Support/Autodesk/Autodesk Fusion 360/API/AddIns/chip_carving_paths_cpp")

install(TARGETS chip_carving_paths_cpp
    LIBRARY DESTINATION "${FUSION_ADDINS_DIR}"
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/chip_carving_paths_cpp.manifest
    DESTINATION "${FUSION_ADDINS_DIR}"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources
    DESTINATION "${FUSION_ADDINS_DIR}"
)

# Custom install target specifically for Fusion 360
# This can be invoked by 'make install-fusion'
add_custom_target(install-fusion
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/install.sh
    DEPENDS chip_carving_paths_cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Installing plugin to Fusion 360 add-ins directory"
)

# Enable testing (optional - only if tests are available)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
    
    # Add test target that builds and runs all tests
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS chip_carving_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running all unit tests"
    )
    
    # Add test target that fails build on test failure
    add_custom_target(test_and_build
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${CMAKE_COMMAND} -E echo "✅ All tests passed! Building plugin..."
        DEPENDS chip_carving_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running tests and building on success"
    )
endif()

# ==============================================================================
# Code Quality and Linting Targets
# ==============================================================================

# Find required tools
find_program(CPPLINT_EXECUTABLE cpplint)
find_program(CLANG_FORMAT_EXECUTABLE clang-format)

# cpplint target - comprehensive C++ style checking
if(CPPLINT_EXECUTABLE)
    add_custom_target(lint
        COMMAND ${CPPLINT_EXECUTABLE}
            --filter=-build/include_subdir,-legal/copyright,-readability/casting,-build/include,-build/namespaces
            --linelength=120
            --recursive
            src/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running cpplint on source files"
    )
    
    # Quick lint - only check critical issues (faster feedback)
    add_custom_target(lint-quick 
        COMMAND ${CPPLINT_EXECUTABLE}
            --filter=-build/include_subdir,-legal/copyright,-readability/casting,-build/include,-build/namespaces,-whitespace/line_length,-whitespace/end_of_line,-whitespace/comments,-whitespace/blank_line
            --recursive
            src/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running quick cpplint (critical issues only)"
    )

    # Lint specific directories
    add_custom_target(lint-core
        COMMAND ${CPPLINT_EXECUTABLE}
            --filter=-build/include_subdir,-legal/copyright,-readability/casting,-build/include,-build/namespaces
            --linelength=120
            --recursive
            src/core/ src/geometry/ src/commands/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running cpplint on core source files"
    )
else()
    message(WARNING "cpplint not found. Install with: pip3 install cpplint")
endif()

# clang-format target - automatic code formatting
if(CLANG_FORMAT_EXECUTABLE)
    # Check if code needs formatting (dry run)
    add_custom_target(format-check
        COMMAND find src/ -name "*.cpp" -o -name "*.h" | xargs ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror --style=file
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking if code formatting is needed"
    )
    
    # Apply automatic formatting 
    add_custom_target(format
        COMMAND find src/ -name "*.cpp" -o -name "*.h" | xargs ${CLANG_FORMAT_EXECUTABLE} -i --style=Google
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Auto-formatting source code with clang-format"
    )
else()
    message(WARNING "clang-format not found. Available in Xcode or install with: brew install clang-format")
endif()

# Combined quality check target
add_custom_target(quality-check
    DEPENDS lint-quick
    COMMENT "Running comprehensive code quality checks"
)

# Development workflow targets
add_custom_target(pre-commit
    DEPENDS quality-check
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Pre-commit checks passed!"
    COMMENT "Run quality checks before committing"
)
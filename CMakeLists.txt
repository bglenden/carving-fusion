cmake_minimum_required(VERSION 3.20)
project(ChipCarvingPathsCppSimple)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type - default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Debug flags - full symbols, no optimization, frame pointers for better stack traces
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG -D_DEBUG -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Release flags - optimized, no debug
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

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
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/increment_version.sh
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
    src/commands/PluginCommandsCreation.cpp
    src/commands/PluginCommandsExecution.cpp
    src/commands/PluginCommandsGeometry.cpp
    src/commands/PluginCommandsImport.cpp
    src/commands/PluginCommandsParameters.cpp
    src/commands/PluginCommandsValidation.cpp
    src/commands/SettingsCommand.cpp
    src/parsers/DesignParser.cpp
    src/geometry/ShapeFactory.cpp
    src/geometry/Leaf.cpp
    src/geometry/TriArc.cpp
    src/geometry/MedialAxisUtilities.cpp
    src/geometry/MedialAxisProcessor.cpp
    src/geometry/VCarvePath.cpp
    src/geometry/VCarveCalculator.cpp
    src/utils/logging.cpp
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
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
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
    
    # Add coverage analysis target (if coverage is enabled)
    option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
    if(ENABLE_COVERAGE)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E echo "Running tests with coverage..."
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage report..."
            COMMAND lcov --capture --directory . --output-file coverage.info
            COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND lcov --list coverage.info
            DEPENDS chip_carving_tests
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running coverage analysis"
        )
    endif()
endif()

# ==============================================================================
# Code Quality and Linting Targets
# ==============================================================================

# Find required tools
find_program(CPPLINT_EXECUTABLE cpplint)
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)

# cpplint target - C++ style and C++14 compatibility checking
if(CPPLINT_EXECUTABLE)
    message(STATUS "Found cpplint: ${CPPLINT_EXECUTABLE}")
    
    # Main lint target using cpplint for C++ issues and file length check
    add_custom_target(lint
        COMMAND ${CPPLINT_EXECUTABLE}
            --recursive
            --extensions=h,cpp
            --headers=h
            --quiet
            src/ include/
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_file_length.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking C++ style, compatibility, and file lengths"
    )
    
    # Quick lint - check only critical issues
    add_custom_target(lint-quick
        COMMAND ${CPPLINT_EXECUTABLE}
            --recursive
            --extensions=h,cpp
            --headers=h
            --quiet
            --filter=-whitespace,-readability/todo,-runtime/references
            src/ include/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Quick C++ checks with cpplint (critical issues only)"
    )
    
    # Verbose lint with all details
    add_custom_target(lint-verbose
        COMMAND ${CPPLINT_EXECUTABLE}
            --recursive
            --extensions=h,cpp
            --headers=h
            --verbose=1
            src/ include/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Verbose C++ checking with cpplint"
    )
else()
    message(WARNING "cpplint not found. Install with: pip3 install cpplint")
    # Create dummy targets so make commands don't fail
    add_custom_target(lint
        COMMAND ${CMAKE_COMMAND} -E echo "cpplint not found. Please install with: pip3 install cpplint"
        COMMENT "Lint target unavailable - cpplint not installed"
    )
    add_custom_target(lint-quick
        COMMAND ${CMAKE_COMMAND} -E echo "cpplint not found. Please install with: pip3 install cpplint"
        COMMENT "Lint-quick target unavailable - cpplint not installed"
    )
endif()

# clang-format target - automatic code formatting
if(CLANG_FORMAT_EXECUTABLE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXECUTABLE}")
    
    # Apply automatic formatting 
    add_custom_target(format
        COMMAND find src/ include/ -name "*.cpp" -o -name "*.h" | 
                xargs ${CLANG_FORMAT_EXECUTABLE} -i --style=file
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Auto-formatting source code with clang-format"
    )
    
    # Check if code needs formatting (dry run)
    add_custom_target(format-check
        COMMAND find src/ include/ -name "*.cpp" -o -name "*.h" | 
                xargs ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror --style=file
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking if code formatting is needed"
    )
else()
    message(WARNING "clang-format not found. Install with: brew install clang-format")
endif()

# clang-tidy target - static analysis (optional, requires LLVM)
if(CLANG_TIDY_EXECUTABLE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE}")
    
    # Comprehensive static analysis
    add_custom_target(lint-analysis
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            -p ${CMAKE_BINARY_DIR}
            --config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy
            src/**/*.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running static analysis with clang-tidy"
    )
else()
    message(STATUS "clang-tidy not found. Install LLVM for static analysis: brew install llvm")
endif()

# Combined quality check target
add_custom_target(quality-check
    DEPENDS lint-quick
    COMMENT "Running code quality checks"
)

# Development workflow targets
add_custom_target(pre-commit
    DEPENDS quality-check
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Pre-commit checks passed!"
    COMMENT "Run quality checks before committing"
)
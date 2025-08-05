# CMakeLists.txt for tests
cmake_minimum_required(VERSION 3.20)
project(ChipCarvingTests)

# Enable testing
enable_testing()

# Find Google Test (system installation)
find_package(GTest REQUIRED)

# Include directories (only src and include, no Fusion SDK needed for unit tests)
include_directories(../src)
include_directories(../include)

# Include OpenVoronoi for standalone medial axis test (system installation)
include_directories(SYSTEM /usr/local/include/openvoronoi)

# Create unified test executable including all tests
add_executable(chip_carving_tests
    core/test_PluginManager.cpp
    adapters/test_MockAdapters.cpp
    adapters/test_SketchArcDrawing.cpp
    # adapters/test_PolygonChaining.cpp  # Temporarily disabled due to Fusion API linkage issues
    geometry/test_Point2D.cpp
    geometry/test_Leaf.cpp
    geometry/test_LeafVisual.cpp
    geometry/test_TriArc.cpp
    geometry/test_TriArcVisual.cpp
    geometry/test_GeometryUtilities.cpp
    geometry/test_MedialAxisUtilities.cpp
    # geometry/test_MedialAxisTruthFiles.cpp - Deprecated (shape-based tests)
    geometry/test_MedialAxisProcessor.cpp
    geometry/test_PolygonExtraction.cpp
    geometry/test_MedialAxisRobustness.cpp
    geometry/test_CoordinateTransform.cpp
    geometry/test_CoordinateSystemRegression.cpp
    geometry/test_SurfaceZDetectionRegression.cpp
    geometry/test_VCarvePath.cpp
    geometry/test_VCarveCalculator.cpp
    parsers/test_DesignParser.cpp
    commands/test_ParameterValidation.cpp
    commands/test_SketchSelectionValidation.cpp
    commands/test_ParameterExtraction.cpp
    commands/test_ToolSelectionLogic.cpp
    commands/test_SelectionValidation.cpp
    core/test_PathGenerationLogic.cpp
    test_ShapeFactory.cpp
    test_SVGGenerator.cpp
    test_main.cpp
    cross_validation_test.cpp
    utils/test_UnitConversion.cpp
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure C++17 compatibility for std::optional
target_compile_definitions(chip_carving_tests PRIVATE
    -DCMAKE_CXX_STANDARD=17
    -D_LIBCPP_ENABLE_CXX17_REMOVED_FEATURES
)

# Add explicit C++17 compile features
target_compile_features(chip_carving_tests PRIVATE cxx_std_17)
set_target_properties(chip_carving_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Find OpenVoronoi library for MedialAxisProcessor tests (system installation)
find_library(OPENVORONOI_LIBRARY 
    NAMES openvoronoi
    PATHS /usr/local/lib/openvoronoi
    NO_DEFAULT_PATH
)

if(NOT OPENVORONOI_LIBRARY)
    message(FATAL_ERROR "OpenVoronoi library not found for MedialAxisProcessor tests")
endif()

# Find Boost for OpenVoronoi
find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Using Boost for tests: ${Boost_INCLUDE_DIRS}")
endif()

# Link with Google Test and OpenVoronoi (needed for MedialAxisProcessor)
target_link_libraries(chip_carving_tests
    GTest::gtest
    GTest::gtest_main
    ${OPENVORONOI_LIBRARY}
    ${Boost_LIBRARIES}
)

# For macOS filesystem support
if(APPLE)
    target_compile_definitions(chip_carving_tests PRIVATE _LIBCPP_ENABLE_CXX17_REMOVED_FEATURES)
endif()

# Add source files to compile (only core logic, no Fusion API dependencies)
target_sources(chip_carving_tests PRIVATE
    ../src/core/PluginManager.cpp
    ../src/geometry/Leaf.cpp
    ../src/geometry/TriArc.cpp
    ../src/geometry/ShapeFactory.cpp
    ../src/geometry/SVGGenerator.cpp
    ../src/geometry/MedialAxisUtilities.cpp
    ../src/geometry/MedialAxisProcessor.cpp
    ../src/parsers/DesignParser.cpp
    ../src/geometry/VCarvePath.cpp
    ../src/geometry/VCarveCalculator.cpp
    mocks/MockLogging.cpp
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(chip_carving_tests)

# Create standalone medial axis test executable (separate from GoogleTest)
add_executable(standalone_medial_axis_test
    standalone_medial_axis_test.cpp
)

# Set C++ standard for standalone test
set_target_properties(standalone_medial_axis_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# OpenVoronoi library already found above

# Link standalone test with OpenVoronoi and required source files
target_link_libraries(standalone_medial_axis_test
    ${OPENVORONOI_LIBRARY}
    ${Boost_LIBRARIES}
)

# Add source files needed for standalone test
target_sources(standalone_medial_axis_test PRIVATE
    ../src/geometry/Leaf.cpp
    ../src/geometry/TriArc.cpp
    ../src/geometry/SVGGenerator.cpp
    ../src/geometry/MedialAxisUtilities.cpp
)

# Add custom target to run standalone test
add_custom_target(run_medial_axis_test
    COMMAND standalone_medial_axis_test
    DEPENDS standalone_medial_axis_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running standalone medial axis test"
)
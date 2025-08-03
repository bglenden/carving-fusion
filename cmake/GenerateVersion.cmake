# GenerateVersion.cmake - Process version.h.in template at build time

# Read version from VERSION file 
file(READ "${SOURCE_DIR}/VERSION" VERSION_CONTENT)
string(STRIP "${VERSION_CONTENT}" VERSION_CONTENT)
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${VERSION_CONTENT})
set(ADDIN_VERSION_MAJOR ${CMAKE_MATCH_1})
set(ADDIN_VERSION_MINOR ${CMAKE_MATCH_2})
set(ADDIN_VERSION_PATCH ${CMAKE_MATCH_3})

# Set build type debug flag
if(NOT DEFINED CMAKE_BUILD_TYPE_DEBUG)
    set(CMAKE_BUILD_TYPE_DEBUG 0)
endif()

# Process the template
configure_file(
    "${SOURCE_DIR}/src/version.h.in"
    "${SOURCE_DIR}/src/version.h"
    @ONLY
)
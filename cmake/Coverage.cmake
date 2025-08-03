# Coverage configuration for C++ tests
# Based on standard gcov/lcov coverage setup

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

# Function to add coverage flags
function(add_coverage_flags target)
    if(ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE --coverage -g -O0)
            target_link_options(${target} PRIVATE --coverage)
        else()
            message(WARNING "Coverage is only supported with GCC or Clang")
        endif()
    endif()
endfunction()

# Function to create coverage target
function(create_coverage_target target test_target)
    if(ENABLE_COVERAGE)
        find_program(LCOV lcov)
        find_program(GENHTML genhtml)
        
        if(LCOV AND GENHTML)
            add_custom_target(${target}
                # Clean previous coverage data
                COMMAND ${LCOV} --directory . --zerocounters
                
                # Run tests
                COMMAND ${test_target}
                
                # Capture coverage data
                COMMAND ${LCOV} --directory . --capture --output-file coverage.info
                
                # Remove system headers and test files from coverage
                COMMAND ${LCOV} --remove coverage.info '/usr/*' '*/tests/*' '*/external/*' '_deps/*' --output-file coverage.info.cleaned
                
                # Generate HTML report
                COMMAND ${GENHTML} -o coverage_report coverage.info.cleaned
                
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${test_target}
                COMMENT "Generating coverage report"
            )
            
            # Print summary
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${LCOV} --summary coverage.info.cleaned
                COMMENT "Coverage summary"
            )
        else()
            message(WARNING "lcov and genhtml are required for coverage reports")
        endif()
    endif()
endfunction()
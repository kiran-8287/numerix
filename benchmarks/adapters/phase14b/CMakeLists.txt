cmake_minimum_required(VERSION 3.16)
project(NUMERIXPhase14BRootAdapter LANGUAGES C)

set(NUMERIX_PHASE14B_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static NUMERIX for the adapter" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Do not duplicate the NUMERIX tests" FORCE)
add_subdirectory("${NUMERIX_PHASE14B_ROOT}" "${CMAKE_BINARY_DIR}/numerix")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(phase14b_root_runner phase14b_root_runner.c)
target_include_directories(phase14b_root_runner PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../phase14")
target_link_libraries(phase14b_root_runner PRIVATE NUMERIX::numerix)
target_compile_options(phase14b_root_runner PRIVATE -O3 -DNDEBUG -Wall -Wextra -Wpedantic)
set_target_properties(phase14b_root_runner PROPERTIES BUILD_RPATH_USE_ORIGIN ON)

option(PHASE14B_WITH_GSL "Enable direct GSL root-method comparisons" ON)
if(PHASE14B_WITH_GSL)
    find_package(GSL 2.8 QUIET)
    if(GSL_FOUND)
        target_compile_definitions(phase14b_root_runner PRIVATE P14_HAVE_GSL)
        target_link_libraries(phase14b_root_runner PRIVATE GSL::gsl GSL::gslcblas)
    else()
        message(STATUS "GSL not found; Phase 14B root comparisons will be NUMERIX-only")
    endif()
endif()

option(PHASE14B_WITH_CMINPACK "Enable CMINPACK best-available root comparison" ON)
if(PHASE14B_WITH_CMINPACK)
    find_path(PHASE14B_CMINPACK_INCLUDE_DIR cminpack.h
        HINTS "${CMINPACK_ROOT}" PATH_SUFFIXES include/cminpack-1 include)
    find_library(PHASE14B_CMINPACK_LIBRARY NAMES cminpack cminpack_s
        HINTS "${CMINPACK_ROOT}" PATH_SUFFIXES lib lib64)
    if(PHASE14B_CMINPACK_INCLUDE_DIR AND PHASE14B_CMINPACK_LIBRARY)
        target_compile_definitions(phase14b_root_runner PRIVATE P14_HAVE_CMINPACK)
        target_include_directories(phase14b_root_runner PRIVATE "${PHASE14B_CMINPACK_INCLUDE_DIR}")
        target_link_libraries(phase14b_root_runner PRIVATE "${PHASE14B_CMINPACK_LIBRARY}" m)
    else()
        message(STATUS "CMINPACK not found; skipping its best-available root comparison")
    endif()
endif()

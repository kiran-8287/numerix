cmake_minimum_required(VERSION 3.16)
project(NUMERIXPhase14COptimizationPilot LANGUAGES C)

set(NUMERIX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "NUMERIX static library" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Do not duplicate project tests" FORCE)
add_subdirectory("${NUMERIX_ROOT}" "${CMAKE_BINARY_DIR}/numerix")

set(NLOPT_ROOT "" CACHE PATH "Prefix containing NLopt 2.10.1")
if(NOT NLOPT_ROOT)
  message(FATAL_ERROR "Set NLOPT_ROOT to the validated NLopt 2.10.1 prefix")
endif()
find_path(NLOPT_INCLUDE_DIR nlopt.h
  HINTS "${NLOPT_ROOT}" PATH_SUFFIXES include NO_DEFAULT_PATH)
find_library(NLOPT_LIBRARY NAMES nlopt
  HINTS "${NLOPT_ROOT}" PATH_SUFFIXES lib lib64 NO_DEFAULT_PATH)
if(NOT NLOPT_INCLUDE_DIR OR NOT NLOPT_LIBRARY)
  message(FATAL_ERROR "NLopt headers or library were not found under NLOPT_ROOT")
endif()

add_executable(optimizer_correctness_pilot optimizer_correctness_pilot.c)
target_include_directories(optimizer_correctness_pilot PRIVATE "${NLOPT_INCLUDE_DIR}")
target_link_libraries(optimizer_correctness_pilot PRIVATE
  NUMERIX::numerix "${NLOPT_LIBRARY}" m)
target_compile_features(optimizer_correctness_pilot PRIVATE c_std_11)
target_compile_options(optimizer_correctness_pilot PRIVATE
  -Wall -Wextra -Wpedantic -Werror)
set_target_properties(optimizer_correctness_pilot PROPERTIES
  BUILD_RPATH "${NLOPT_ROOT}/lib64"
  BUILD_RPATH_USE_ORIGIN OFF)

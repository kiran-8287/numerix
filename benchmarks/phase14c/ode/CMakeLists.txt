cmake_minimum_required(VERSION 3.16)
project(NUMERIXPhase14CODEPilot LANGUAGES C)

set(NUMERIX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "NUMERIX static library" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Do not duplicate project tests" FORCE)
add_subdirectory("${NUMERIX_ROOT}" "${CMAKE_BINARY_DIR}/numerix")

set(SUNDIALS_ROOT "" CACHE PATH "Prefix containing the pinned SUNDIALS build")
if(NOT SUNDIALS_ROOT)
  message(FATAL_ERROR "Set SUNDIALS_ROOT to the validated 7.9.0 installation")
endif()
find_package(SUNDIALS 7.9.0 EXACT REQUIRED CONFIG
  PATHS "${SUNDIALS_ROOT}/lib64/cmake/sundials"
  NO_DEFAULT_PATH)

if(NOT TARGET SUNDIALS::arkode_shared OR NOT TARGET SUNDIALS::nvecserial_shared)
  message(FATAL_ERROR "SUNDIALS 7.9.0 must provide serial ARKODE and N_Vector targets")
endif()

add_executable(ode_sundials_pilot ode_sundials_pilot.c)
target_link_libraries(ode_sundials_pilot PRIVATE
  NUMERIX::numerix
  SUNDIALS::arkode_shared
  SUNDIALS::nvecserial_shared)
target_compile_features(ode_sundials_pilot PRIVATE c_std_11)
target_compile_options(ode_sundials_pilot PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(ode_sundials_pilot PROPERTIES BUILD_RPATH_USE_ORIGIN ON)

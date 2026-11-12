cmake_minimum_required(VERSION 3.16)
project(NUMERIXPhase14Adapters LANGUAGES C)

set(NUMERIX_PHASE14_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "NUMERIX uses a static library" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Do not duplicate the NUMERIX test suite in the adapter build" FORCE)
add_subdirectory("${NUMERIX_PHASE14_ROOT}" "${CMAKE_BINARY_DIR}/numerix")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PHASE14_WITH_GSL "Enable the GSL adapter when GSL is available" ON)
option(PHASE14_WITH_CMINPACK "Enable the CMINPACK adapter when available" ON)
option(PHASE14_WITH_NLOPT "Enable the NLopt adapter when available" ON)
option(PHASE14_WITH_SUNDIALS "Enable the SUNDIALS adapter when available" ON)
option(PHASE14_WITH_REFERENCE_LAPACK "Build the reference LAPACKE adapter when available" ON)
option(PHASE14_WITH_OPENBLAS "Build the OpenBLAS-backed LAPACKE adapter when available" ON)

set(PHASE14_ADAPTER_SOURCE "${CMAKE_CURRENT_LIST_DIR}/phase14_runner.c")
set(PHASE14_ADAPTER_INCLUDE "${CMAKE_CURRENT_LIST_DIR}")

function(phase14_configure_runner target include_general_libraries)
    add_executable(${target} "${PHASE14_ADAPTER_SOURCE}")
    target_include_directories(${target} PRIVATE "${PHASE14_ADAPTER_INCLUDE}")
    target_link_libraries(${target} PRIVATE NUMERIX::numerix)
    target_compile_options(${target} PRIVATE -O3 -DNDEBUG -Wall -Wextra -Wpedantic)
    set_target_properties(${target} PROPERTIES BUILD_RPATH_USE_ORIGIN ON)
    if(include_general_libraries AND PHASE14_GSL_FOUND)
        target_compile_definitions(${target} PRIVATE P14_HAVE_GSL)
        target_link_libraries(${target} PRIVATE GSL::gsl GSL::gslcblas)
    endif()
    if(include_general_libraries AND PHASE14_CMINPACK_FOUND)
        target_compile_definitions(${target} PRIVATE P14_HAVE_CMINPACK)
        target_include_directories(${target} PRIVATE "${PHASE14_CMINPACK_INCLUDE_DIR}")
        target_link_libraries(${target} PRIVATE "${PHASE14_CMINPACK_LIBRARY}" m)
    endif()
    if(include_general_libraries AND PHASE14_NLOPT_FOUND)
        target_compile_definitions(${target} PRIVATE P14_HAVE_NLOPT)
        target_include_directories(${target} PRIVATE "${PHASE14_NLOPT_INCLUDE_DIR}")
        target_link_libraries(${target} PRIVATE "${PHASE14_NLOPT_LIBRARY}")
    endif()
    if(include_general_libraries AND PHASE14_SUNDIALS_FOUND)
        target_compile_definitions(${target} PRIVATE P14_HAVE_SUNDIALS)
        target_include_directories(${target} PRIVATE "${PHASE14_SUNDIALS_INCLUDE_DIR}")
        target_link_libraries(${target} PRIVATE ${PHASE14_SUNDIALS_LIBRARIES})
    endif()
endfunction()

set(PHASE14_GSL_FOUND OFF)
if(PHASE14_WITH_GSL)
    find_package(GSL 2.8 QUIET)
    if(GSL_FOUND)
        set(PHASE14_GSL_FOUND ON)
    else()
        message(STATUS "GSL not found; building adapters without GSL")
    endif()
endif()

set(PHASE14_CMINPACK_FOUND OFF)
if(PHASE14_WITH_CMINPACK)
    find_path(PHASE14_CMINPACK_INCLUDE_DIR cminpack.h
        HINTS "${CMINPACK_ROOT}" PATH_SUFFIXES include/cminpack-1 include)
    find_library(PHASE14_CMINPACK_LIBRARY NAMES cminpack cminpack_s
        HINTS "${CMINPACK_ROOT}" PATH_SUFFIXES lib lib64)
    if(PHASE14_CMINPACK_INCLUDE_DIR AND PHASE14_CMINPACK_LIBRARY)
        set(PHASE14_CMINPACK_FOUND ON)
    else()
        message(STATUS "CMINPACK not found; building adapters without CMINPACK")
    endif()
endif()

set(PHASE14_NLOPT_FOUND OFF)
if(PHASE14_WITH_NLOPT)
    find_path(PHASE14_NLOPT_INCLUDE_DIR nlopt.h
        HINTS "${NLOPT_ROOT}" PATH_SUFFIXES include)
    find_library(PHASE14_NLOPT_LIBRARY NAMES nlopt
        HINTS "${NLOPT_ROOT}" PATH_SUFFIXES lib lib64)
    if(PHASE14_NLOPT_INCLUDE_DIR AND PHASE14_NLOPT_LIBRARY)
        set(PHASE14_NLOPT_FOUND ON)
    else()
        message(STATUS "NLopt not found; building adapters without NLopt")
    endif()
endif()

set(PHASE14_SUNDIALS_FOUND OFF)
if(PHASE14_WITH_SUNDIALS)
    find_path(PHASE14_SUNDIALS_INCLUDE_DIR cvode/cvode.h
        HINTS "${SUNDIALS_ROOT}" PATH_SUFFIXES include)
    find_library(PHASE14_SUNDIALS_CVODE NAMES sundials_cvode
        HINTS "${SUNDIALS_ROOT}" PATH_SUFFIXES lib lib64)
    find_library(PHASE14_SUNDIALS_ARKODE NAMES sundials_arkode
        HINTS "${SUNDIALS_ROOT}" PATH_SUFFIXES lib lib64)
    find_library(PHASE14_SUNDIALS_NVEC NAMES sundials_nvecserial
        HINTS "${SUNDIALS_ROOT}" PATH_SUFFIXES lib lib64)
    find_library(PHASE14_SUNDIALS_NONLINSOL NAMES sundials_sunnonlinsolfixedpoint
        HINTS "${SUNDIALS_ROOT}" PATH_SUFFIXES lib lib64)
    find_library(PHASE14_SUNDIALS_CORE NAMES sundials_core
        HINTS "${SUNDIALS_ROOT}" PATH_SUFFIXES lib lib64)
    if(PHASE14_SUNDIALS_INCLUDE_DIR AND PHASE14_SUNDIALS_CVODE AND
       PHASE14_SUNDIALS_ARKODE AND PHASE14_SUNDIALS_NVEC AND
       PHASE14_SUNDIALS_NONLINSOL AND PHASE14_SUNDIALS_CORE)
        set(PHASE14_SUNDIALS_FOUND ON)
        set(PHASE14_SUNDIALS_LIBRARIES ${PHASE14_SUNDIALS_ARKODE}
            ${PHASE14_SUNDIALS_CVODE} ${PHASE14_SUNDIALS_NVEC}
            ${PHASE14_SUNDIALS_NONLINSOL} ${PHASE14_SUNDIALS_CORE} m)
    else()
        message(STATUS "SUNDIALS serial CVODE/ARKODE not found; building adapters without SUNDIALS")
    endif()
endif()

phase14_configure_runner(phase14_runner TRUE)

if(PHASE14_WITH_REFERENCE_LAPACK)
    find_path(PHASE14_REFERENCE_LAPACKE_INCLUDE_DIR lapacke.h
        HINTS "${REFERENCE_LAPACK_ROOT}" PATH_SUFFIXES include)
    find_library(PHASE14_REFERENCE_LAPACKE NAMES lapacke
        HINTS "${REFERENCE_LAPACK_ROOT}" PATH_SUFFIXES lib lib64)
    find_library(PHASE14_REFERENCE_LAPACK NAMES lapack
        HINTS "${REFERENCE_LAPACK_ROOT}" PATH_SUFFIXES lib lib64)
    find_library(PHASE14_REFERENCE_CBLAS NAMES cblas
        HINTS "${REFERENCE_LAPACK_ROOT}" PATH_SUFFIXES lib lib64)
    find_library(PHASE14_REFERENCE_BLAS NAMES blas
        HINTS "${REFERENCE_LAPACK_ROOT}" PATH_SUFFIXES lib lib64)
    if(PHASE14_REFERENCE_LAPACKE_INCLUDE_DIR AND PHASE14_REFERENCE_LAPACKE AND
       PHASE14_REFERENCE_LAPACK AND PHASE14_REFERENCE_CBLAS AND PHASE14_REFERENCE_BLAS)
        phase14_configure_runner(phase14_runner_reference_lapack FALSE)
        target_compile_definitions(phase14_runner_reference_lapack PRIVATE
            P14_HAVE_LAPACKE P14_LAPACK_BACKEND_REFERENCE)
        target_include_directories(phase14_runner_reference_lapack PRIVATE
            "${PHASE14_REFERENCE_LAPACKE_INCLUDE_DIR}")
        target_link_libraries(phase14_runner_reference_lapack PRIVATE
            "${PHASE14_REFERENCE_LAPACKE}" "${PHASE14_REFERENCE_LAPACK}"
            "${PHASE14_REFERENCE_CBLAS}" "${PHASE14_REFERENCE_BLAS}" gfortran)
    else()
        message(STATUS "Reference LAPACKE not found; skipping its runner")
    endif()
endif()

if(PHASE14_WITH_OPENBLAS)
    find_path(PHASE14_OPENBLAS_INCLUDE_DIR lapacke.h
        HINTS "${OPENBLAS_ROOT}" PATH_SUFFIXES include)
    find_library(PHASE14_OPENBLAS_LIBRARY NAMES openblas_haswellp-r0.3.34 openblas
        HINTS "${OPENBLAS_ROOT}" PATH_SUFFIXES lib lib64)
    if(PHASE14_OPENBLAS_INCLUDE_DIR AND PHASE14_OPENBLAS_LIBRARY)
        phase14_configure_runner(phase14_runner_openblas FALSE)
        target_compile_definitions(phase14_runner_openblas PRIVATE
            P14_HAVE_LAPACKE P14_HAVE_OPENBLAS P14_LAPACK_BACKEND_OPENBLAS)
        target_include_directories(phase14_runner_openblas PRIVATE
            "${PHASE14_OPENBLAS_INCLUDE_DIR}")
        target_link_libraries(phase14_runner_openblas PRIVATE "${PHASE14_OPENBLAS_LIBRARY}")
    else()
        message(STATUS "OpenBLAS LAPACKE not found; skipping its runner")
    endif()
endif()

message(STATUS "Phase 14 adapters: GSL=${PHASE14_GSL_FOUND}; CMINPACK=${PHASE14_CMINPACK_FOUND}; NLopt=${PHASE14_NLOPT_FOUND}; SUNDIALS=${PHASE14_SUNDIALS_FOUND}")

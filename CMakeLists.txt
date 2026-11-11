cmake_minimum_required(VERSION 3.16)
project(NUMERIX VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(numerix STATIC
    src/core/numerix_core.c
    src/core/numerix_platform.c
    src/algorithms/root_finding/root_finding.c
    src/algorithms/linear_algebra/linear_algebra.c
    src/algorithms/differentiation/differentiation.c
    src/algorithms/interpolation/interpolation.c
    src/algorithms/integration/integration.c
    src/algorithms/optimization/optimization.c
    src/algorithms/ode/ode.c
    src/expression/lexer.c
    src/expression/parser.c
    src/expression/evaluator.c
    src/expression/expression.c
    src/piecewise/piecewise.c
    src/piecewise/analysis.c
    src/piecewise/adapters.c
    src/intelligence/problem.c
    src/intelligence/recommendation.c
    src/intelligence/root_execution.c
    src/intelligence/portfolio.c
    src/intelligence/execution.c
    src/execution/managed_execution.c
    src/diagnostics/diagnostics.c
    src/diagnostics/diagnostic_analysis.c
    src/root_discovery/root_discovery.c
)
target_include_directories(numerix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(numerix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/internal)
if(NOT WIN32)
    target_link_libraries(numerix PUBLIC m)
endif()
if(MSVC)
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "/experimental:c11atomics")
    check_c_source_compiles("#include <stdatomic.h>
int main(void) { atomic_int value = 0; return atomic_load(&value); }"
        NUMERIX_MSVC_C11_ATOMICS)
    unset(CMAKE_REQUIRED_FLAGS)
    if(NOT NUMERIX_MSVC_C11_ATOMICS)
        message(FATAL_ERROR "NUMERIX requires MSVC C11 atomics (VS 2022 17.5+ with /experimental:c11atomics)")
    endif()
    target_compile_options(numerix PRIVATE /W4 /experimental:c11atomics)
else()
    target_compile_options(numerix PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(numerix_cli cli/main.c)
target_link_libraries(numerix_cli PRIVATE numerix)

add_executable(numerix_expression_demo examples/expression_demo.c)
target_link_libraries(numerix_expression_demo PRIVATE numerix)

add_executable(numerix_piecewise_demo examples/piecewise_demo.c)
target_link_libraries(numerix_piecewise_demo PRIVATE numerix)

add_executable(numerix_intelligence_demo examples/intelligence_demo.c)
target_link_libraries(numerix_intelligence_demo PRIVATE numerix)

add_executable(numerix_managed_execution_demo examples/managed_execution_demo.c)
target_link_libraries(numerix_managed_execution_demo PRIVATE numerix)

add_executable(numerix_root_discovery_demo examples/root_discovery_demo.c)
target_link_libraries(numerix_root_discovery_demo PRIVATE numerix)

add_executable(numerix_diagnostics_benchmark EXCLUDE_FROM_ALL
    benchmarks/diagnostics_benchmark.c)
target_link_libraries(numerix_diagnostics_benchmark PRIVATE numerix)

add_executable(numerix_root_discovery_benchmark EXCLUDE_FROM_ALL
    benchmarks/root_discovery_benchmark.c)
target_link_libraries(numerix_root_discovery_benchmark PRIVATE numerix)

include(CTest)
if(BUILD_TESTING)
    add_executable(numerix_tests tests/test_numerix.c)
    target_link_libraries(numerix_tests PRIVATE numerix)
    add_executable(numerix_phase1_tests tests/test_phase1.c)
    target_link_libraries(numerix_phase1_tests PRIVATE numerix)
    add_executable(numerix_expression_tests tests/test_expression.c)
    target_link_libraries(numerix_expression_tests PRIVATE numerix)
    add_executable(numerix_audit_regression_tests tests/test_audit_regressions.c)
    target_link_libraries(numerix_audit_regression_tests PRIVATE numerix)
    add_executable(numerix_piecewise_tests tests/test_piecewise.c)
    target_link_libraries(numerix_piecewise_tests PRIVATE numerix)
    add_executable(numerix_intelligence_tests tests/test_intelligence.c)
    target_link_libraries(numerix_intelligence_tests PRIVATE numerix)
    add_executable(numerix_execution_tests tests/test_execution.c)
    target_link_libraries(numerix_execution_tests PRIVATE numerix)
    add_executable(numerix_diagnostics_tests tests/test_diagnostics.c)
    target_link_libraries(numerix_diagnostics_tests PRIVATE numerix)
    add_executable(numerix_root_discovery_tests tests/test_root_discovery.c)
    target_link_libraries(numerix_root_discovery_tests PRIVATE numerix)
    add_executable(numerix_phase8_integration_tests tests/test_phase8_integration.c)
    target_link_libraries(numerix_phase8_integration_tests PRIVATE numerix)
    if(NOT MSVC)
        target_compile_options(numerix_diagnostics_benchmark PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_root_discovery_benchmark PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_phase1_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_expression_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_audit_regression_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_piecewise_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_intelligence_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_execution_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_diagnostics_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_root_discovery_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        target_compile_options(numerix_phase8_integration_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    add_test(NAME numerix_unit COMMAND numerix_tests unit)
    add_test(NAME numerix_integration COMMAND numerix_tests integration)
    add_test(NAME numerix_root_finding COMMAND numerix_phase1_tests root_finding)
    add_test(NAME numerix_linear_algebra COMMAND numerix_phase1_tests linear_algebra)
    add_test(NAME numerix_differentiation COMMAND numerix_phase1_tests differentiation)
    add_test(NAME numerix_interpolation COMMAND numerix_phase1_tests interpolation)
    add_test(NAME numerix_phase1_integration COMMAND numerix_phase1_tests integration)
    add_test(NAME numerix_optimization COMMAND numerix_phase1_tests optimization)
    add_test(NAME numerix_ode COMMAND numerix_phase1_tests ode)
    add_test(NAME numerix_expression_engine COMMAND numerix_expression_tests)
    add_test(NAME numerix_audit_regressions COMMAND numerix_audit_regression_tests)
    add_test(NAME numerix_piecewise COMMAND numerix_piecewise_tests)
    add_test(NAME numerix_intelligence COMMAND numerix_intelligence_tests)
    add_test(NAME numerix_execution COMMAND numerix_execution_tests)
    add_test(NAME numerix_diagnostics COMMAND numerix_diagnostics_tests)
    add_test(NAME numerix_root_discovery COMMAND numerix_root_discovery_tests)
    add_test(NAME numerix_phase8_integration COMMAND numerix_phase8_integration_tests)
endif()

# GSL is used only by the opt-in comparison executable, never by numerix itself.
option(NUMERIX_BUILD_COMPARISON_BENCHMARK "Build the Phase 9 benchmark" OFF)
option(NUMERIX_BENCHMARK_ALLOCATIONS "Build an untimed GNU/ELF allocation probe" OFF)
if(NUMERIX_BUILD_COMPARISON_BENCHMARK)
    find_package(GSL QUIET)
    add_executable(numerix_comparison_benchmark benchmarks/comparison.c)
    target_link_libraries(numerix_comparison_benchmark PRIVATE numerix)
    if(GSL_FOUND)
        target_compile_definitions(numerix_comparison_benchmark PRIVATE NUMERIX_HAVE_GSL)
        target_link_libraries(numerix_comparison_benchmark PRIVATE GSL::gsl GSL::gslcblas)
    else()
        message(STATUS "GSL unavailable: comparison benchmark will run NUMERIX cases only")
    endif()
    if(NOT MSVC)
        target_compile_options(numerix_comparison_benchmark PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    if(NUMERIX_BENCHMARK_ALLOCATIONS)
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR MSVC)
            message(FATAL_ERROR "Allocation probing requires the GNU/ELF linker on Linux")
        endif()
        if(GSL_FOUND AND (NOT GSL_LIBRARY MATCHES "\\.a$" OR
                          NOT GSL_CBLAS_LIBRARY MATCHES "\\.a$"))
            message(FATAL_ERROR "Allocation probing requires static GSL and CBLAS archives")
        endif()
        add_executable(numerix_comparison_memory
            benchmarks/comparison.c benchmarks/allocation_tracker.c)
        target_compile_definitions(numerix_comparison_memory PRIVATE NUMERIX_BENCH_ALLOCATIONS)
        target_link_libraries(numerix_comparison_memory PRIVATE numerix)
        target_link_options(numerix_comparison_memory PRIVATE
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free)
        target_compile_options(numerix_comparison_memory PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(GSL_FOUND)
            target_compile_definitions(numerix_comparison_memory PRIVATE NUMERIX_HAVE_GSL)
            target_link_libraries(numerix_comparison_memory PRIVATE GSL::gsl GSL::gslcblas)
        endif()
    endif()
endif()

if(BUILD_TESTING)
    add_executable(numerix_release_regression_tests tests/test_release_regressions.c)
    target_link_libraries(numerix_release_regression_tests PRIVATE numerix)
    if(NOT MSVC)
        target_compile_options(numerix_release_regression_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    add_test(NAME numerix_release_regressions COMMAND numerix_release_regression_tests)
endif()

include(GNUInstallDirs)
install(TARGETS numerix ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/numerix.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

if(BUILD_TESTING)
    add_test(NAME numerix_cli_startup COMMAND numerix_cli)
    add_test(NAME numerix_cli_help COMMAND numerix_cli --help)
    add_test(NAME numerix_cli_expression COMMAND numerix_cli --eval "sin(pi/2)+x^2" 2)
    set_tests_properties(numerix_cli_expression PROPERTIES PASS_REGULAR_EXPRESSION "= 5")
    add_test(NAME numerix_cli_root COMMAND numerix_cli --demo)
    set_tests_properties(numerix_cli_root PROPERTIES PASS_REGULAR_EXPRESSION "Diagnostic residual")
    add_test(NAME numerix_cli_discovery COMMAND numerix_cli --roots "(x-1)*(x-2)" 0 3)
    set_tests_properties(numerix_cli_discovery PROPERTIES PASS_REGULAR_EXPRESSION "2 roots")
    add_test(NAME numerix_cli_piecewise COMMAND numerix_cli --piecewise-demo)
    add_test(NAME numerix_cli_invalid COMMAND numerix_cli --root "x" nan 1)
    add_test(NAME numerix_cli_domain_error COMMAND numerix_cli --eval "sqrt(x)" -1)
    set_tests_properties(numerix_cli_invalid numerix_cli_domain_error PROPERTIES WILL_FAIL TRUE)
    foreach(demo expression piecewise intelligence managed_execution root_discovery)
        add_test(NAME numerix_example_${demo} COMMAND numerix_${demo}_demo)
    endforeach()
endif()

# Optional allocation fault tests exercise nested search/diagnostic cleanup too.
if(NUMERIX_BENCHMARK_ALLOCATIONS AND NUMERIX_BUILD_COMPARISON_BENCHMARK AND BUILD_TESTING)
    add_executable(numerix_allocation_failure_tests
        tests/test_allocation_failures.c benchmarks/allocation_tracker.c)
    target_link_libraries(numerix_allocation_failure_tests PRIVATE numerix)
    target_compile_options(numerix_allocation_failure_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    target_link_options(numerix_allocation_failure_tests PRIVATE
        -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free)
    add_test(NAME numerix_allocation_failures COMMAND numerix_allocation_failure_tests)
endif()

install(DIRECTORY documentation/api/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/api)
set(CPACK_PACKAGE_NAME "numerix")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C11 numerical library and public API documentation")
set(CPACK_GENERATOR "TGZ")
include(CPack)

if(BUILD_TESTING)
    add_executable(numerix_accuracy_tests tests/test_accuracy.c)
    target_link_libraries(numerix_accuracy_tests PRIVATE numerix)
    if(NOT MSVC)
        target_compile_options(numerix_accuracy_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    add_test(NAME numerix_accuracy COMMAND numerix_accuracy_tests)
endif()

if(BUILD_TESTING)
    add_executable(numerix_portability_probe tests/portability_probe.c)
    target_link_libraries(numerix_portability_probe PRIVATE numerix)
    add_test(NAME numerix_portability_layout COMMAND numerix_portability_probe)
endif()
